/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Christophe Robin <crobin@php.net>                            |
  +----------------------------------------------------------------------+

  $Id$
  $Revision$
*/

#include "php_spidermonkey.h"
#include "zend_exceptions.h"


/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	TSRMLS_FETCH();

	if ((report->flags & JSREPORT_WARNING) || (report->flags & JSREPORT_STRICT)) {
		// emit a warning
		php_error_docref(NULL, report->flags & JSREPORT_WARNING ? E_WARNING : E_STRICT, message);
	} else {
		/* throw error */
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), (char *) message, report->errorNumber);
	}
}

/* this function set a property on an object */
void php_jsobject_set_property(JSContext *ctx, JSObject *obj, char *property_name, zval *val)
{
	jsval jval;
	
	PHPJS_START(ctx);

	/* first convert zval to jsval */
	zval_to_jsval(val, ctx, &jval);

	/* no ref behavior, just set a property */
	JSBool res = JS_SetProperty(ctx, obj, property_name, &jval);
	
	PHPJS_END(ctx);
}


// call "PHP function" from JS and convert "return value of PHP func" to jsval
/* all function calls are mapped through this unique function */
JSBool generic_call(JSContext *cx, unsigned argc, jsval *vp)
{
	TSRMLS_FETCH();
	JSFunction				*func;
	JSString				*jfunc_name;
	JSClass					*jclass;
	zend_string				*func_name;
	zval					*params = NULL;
	zval					retval;
	php_callback			*callback;
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	int						i;
	JSObject				*obj;
	/*JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);*/

	/* first retrieve function name */
	JS::CallArgs argv = JS::CallArgsFromVp(argc, vp);

	func = JS_ValueToFunction(cx, argv.calleev());
	jfunc_name = JS_GetFunctionId(func);
	/* because version 1.8.5 supports unicode, we must encode strings */

	char * jsString  = JS_EncodeString(cx, jfunc_name);
	func_name = zend_string_init(jsString, strlen(jsString), 0);
	/* free function name */
	JS_free(cx, jsString);

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jclass = &intern->script_class;

	obj = &(argv.thisv().get().toObject());
	if (obj == nullptr) {
		obj = JS_GetGlobalForScopeChain(intern->ct);
	}

	if (obj == intern->obj) {
		jclass =&intern->global_class;
	}
	
	if ((jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, jclass, NULL)) == NULL)
	{
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function table", 0);
	}

	/* search for function callback */
	callback = (php_callback*)zend_hash_find_ptr(jsref->ht, func_name);
	if (callback == NULL) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0);
	}
	
	/* ready parameters */
	params = (zval*)ecalloc(1, argc * sizeof(zval));
	for (i = 0; i < argc; i++)
	{
		zval *val = &(params[i]);
		jsval_to_zval(val, cx, JS::MutableHandleValue::fromMarkedLocation(&argv[i]));
	}

	if (argc == 0) {
		callback->fci.params = NULL;
	} else {
		callback->fci.params = params;
	}

	callback->fci.param_count		= argc;
	callback->fci.retval	= &retval;

	zend_call_function(&callback->fci, &callback->fci_cache);

	/* call ended, clean */
	for (i = 0; i < argc; i++)
	{
		zval *val = &(params[i]);
		zval_dtor(val);
	}

	if(Z_TYPE(retval) == IS_NULL) 
	{
		argv.rval().get().setNull();
	} 
	else 
	{
		//  convert "return value of PHP func" to jsval	
		zval_to_jsval(&retval, cx, argv.rval().address());
	}

	efree(params);
	zval_dtor(&retval);
	zend_string_release(func_name);

	return JS_TRUE;
}

/* this native is used for class constructors */
JSBool generic_constructor(JSContext *cx, unsigned argc, jsval *vp)
{
	TSRMLS_FETCH();
	JSFunction				*jclass;
	JSString				*jclass_name;
	zend_string					*class_name;
	zval					*params, retval;
	zend_class_entry		*ce;
	zval					cobj;
	/*JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);*/

	php_jscontext_object	*intern;
	int						i;

	JS::CallArgs argv = JS::CallArgsFromVp(argc, vp);

	jclass = JS_ValueToFunction(cx, argv.calleev());
	jclass_name = JS_GetFunctionId(jclass);
	/* because version 1.8.5 supports unicode, we must encode strings */
	char * jsString  = JS_EncodeString(cx, jclass_name);
	class_name = zend_string_init(jsString, strlen(jsString), 0);
	/* free class name */
	JS_free(cx, jsString);

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	
	/* search for class entry */
	ce = (zend_class_entry*)zend_hash_find_ptr(intern->ec_ht, class_name);
	if (ce == NULL) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0);
	}
	
	if (ce->constructor)
	{
		zend_fcall_info			fci;
		zend_fcall_info_cache	fcc;

		if (!(ce->constructor->common.fn_flags & ZEND_ACC_PUBLIC)) {
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0, "Access to non-public constructor");
		}

		object_init_ex(&cobj, ce);

		/* ready parameters */
		params = (zval*)emalloc(argc * sizeof(zval));
		for (i = 0; i < argc; i++)
		{
			zval *val = &(params[i]);
			jsval_to_zval(val, cx, JS::MutableHandleValue::fromMarkedLocation(&argv[i]));
		}

		fci.size			= sizeof(fci);
		fci.object		= Z_OBJ(cobj);
		fci.retval = &retval;
		fci.params			= params;
		fci.param_count		= argc;
		fci.no_separation	= 1;

		fcc.initialized		= 1;
		fcc.function_handler= ce->constructor;
		fcc.calling_scope	= zend_get_executed_scope();
		fcc.called_scope	= Z_OBJCE(cobj);
		//fcc.called_scope	= ce; // todo: any difference to Z_OBJCE(cobj)?
		fcc.object		= Z_OBJ(cobj);

		if (zend_call_function(&fci, &fcc) == FAILURE)
		{
			/* call ended, clean */
			for (i = 0; i < argc; i++)
			{
				zval *eval = &(params[i]);
				zval_dtor(eval);
			}
			
			efree(params);
			zval_dtor(&cobj);
			/* TODO: failed */
			argv.rval().setNull();
			return JS_FALSE;
		}

		/* call ended, clean */
		for (i = 0; i < argc; i++)
		{
			zval *eval = &(params[i]);
			zval_dtor(eval);
		}

		zval_dtor(&retval);
		zval_to_jsval(&cobj, cx, argv.rval().address());
	
		efree(params);
		zval_dtor(&cobj);
	}
	else
	{
		object_init_ex(&cobj, ce);
		zval_to_jsval(&cobj, cx, argv.rval().address());
	zval_dtor(&cobj);
	}

	zend_string_free(class_name);

	return JS_TRUE;
}

JSBool JS_ResolvePHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id)
{
	/* always return true, as PHP doesn't use any resolver */
	return JS_TRUE;
}

JSBool JS_PropertySetterPHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JSBool strict, JS::MutableHandle<JS::Value> vp)
{
	TSRMLS_FETCH();
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;
	JSClass					*jclass;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jclass = &intern->script_class;

	if (obj == intern->obj) {
		jclass =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, jclass, NULL);

	if (jsref != NULL)
	{
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval val;

			/* 1.8.5 uses reals jsid for id, we need to convert it */
			jsval rid;
			JS_IdToValue(cx, id, &rid);
			str = JS_ValueToString(cx, rid);
			/* because version 1.8.5 supports unicode, we must encode strings */
			prop_name = JS_EncodeString(cx, str);

			jsval_to_zval(&val, cx, vp);

			zend_update_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), &val);
			zval_dtor(&val);

			/* free prop name */
			JS_free(cx, prop_name);
		}
	}

	return JS_TRUE;
}

JSBool JS_PropertyGetterPHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
        JS::MutableHandle<JS::Value> vp)
{
	TSRMLS_FETCH();
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;
	JSClass					*jclass;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jclass = &intern->script_class;
	
	if (obj == intern->obj) {
		jclass =&intern->global_class;
	}
	
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	if (jsref != NULL) 
	{
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval *val = NULL;
            JSBool has_property;

			/* 1.8.5 uses reals jsid for id, we need to convert it */
			jsval rid;
			JS_IdToValue(cx, id, &rid);
			str = JS_ValueToString(cx, rid);
			/* because version 1.8.5 supports unicode, we must encode strings */
			prop_name = JS_EncodeString(cx, str);

            has_property = JS_FALSE;
            if (JS_HasProperty(cx, obj, prop_name, &has_property) && (has_property == JS_TRUE)) {
                if (JS_LookupProperty(cx, obj, prop_name, vp.address()) == JS_TRUE) {
					/* free prop name */
					JS_free(cx, prop_name);
                    return JS_TRUE;
                }
            }

			val = zend_read_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), 1, NULL);

			/* free prop name */
			JS_free(cx, prop_name);

			
			if (val != &EG(uninitialized_zval)) {
				zval_to_jsval(val, cx, vp.address());
				return JS_TRUE;
			}
		}
	}
	
	return JS_TRUE;
}

/* This is called when a JSObject is destroyed by the GC or when the context
 * is detroyed */
void JS_FinalizePHP(JSFreeOp *fop, JSObject *obj)
{
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;

	// it is safe to use JS_GetPrivate in a finalizer.
	jsref = (php_jsobject_ref*)JS_GetPrivate(obj);

	/* destroy ref object */
	if (jsref != NULL)
	{
		/* free the functions hash table */
		if (jsref->ht != NULL)
		{
			HashTable* ht = jsref->ht;
			zend_string *string_key = NULL;
			zend_ulong num_key = 0;
			zval *zval_callback;

			ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, string_key, zval_callback) {

				php_callback *callback = (php_callback*)Z_PTR_P(zval_callback);
				efree(callback);

			} ZEND_HASH_FOREACH_END();
			/* destroy hashtable */
			zend_hash_destroy(jsref->ht);
			FREE_HASHTABLE(jsref->ht);
		}

		/* remove reference to object and call ptr dtor */
		if (jsref->obj != NULL)
		{
			zval_dtor(jsref->obj);
		}

		/* then destroy JSRef */
		efree(jsref);
	}
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
