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
	/* throw error */
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), (char *) message, 0 TSRMLS_CC);
}

/* this function set a property on an object */
void php_jsobject_set_property(JSContext *ctx, JSObject *obj, char *property_name, zval *val TSRMLS_DC)
{
	jsval jval;

	/* first convert zval to jsval */
	zval_to_jsval(val, ctx, &jval TSRMLS_CC);

	/* no ref behavior, just set a property */
	JS_SetProperty(ctx, obj, property_name, &jval);
}

/* all function calls are mapped through this unique function */
JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	TSRMLS_FETCH();
	JSFunction				*func;
	JSString				*jfunc_name;
	char					*func_name;
	zval					***params, *retval_ptr = NULL;
	php_callback			*callback;
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	int						i;

	/* first retrieve function name */
	func = JS_ValueToFunction(cx, ((argv)[-2]));
	jfunc_name = JS_GetFunctionId(func);
	func_name = JS_GetStringBytes(jfunc_name);

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);

	if ((jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL)) == 0)
	{
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function table", 0 TSRMLS_CC);
	}

	/* search for function callback */
	if (zend_hash_find(jsref->ht, func_name, strlen(func_name), (void**)&callback) == FAILURE) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0 TSRMLS_CC);
	}

	/* ready parameters */
	params = emalloc(argc * sizeof(zval**));
	for (i = 0; i < argc; i++)
	{
		zval **val = emalloc(sizeof(zval*));
		MAKE_STD_ZVAL(*val);
		jsval_to_zval(*val, cx, &argv[i]);
		params[i] = val;
	}

	callback->fci.params			= params;
	callback->fci.param_count		= argc;
	callback->fci.retval_ptr_ptr	= &retval_ptr;

	zend_call_function(&callback->fci, NULL TSRMLS_CC);

	/* call ended, clean */
	for (i = 0; i < argc; i++)
	{
		zval **eval;
		eval = params[i];
		zval_ptr_dtor(eval);
		efree(eval);
	}

	if (retval_ptr != NULL)
	{
		zval_to_jsval(retval_ptr, cx, rval TSRMLS_CC);
		zval_ptr_dtor(&retval_ptr);
	}
	else
	{
		*rval = JSVAL_NULL;
	}
	
	efree(params);

	return JS_TRUE;
}

/* this native is used for class constructors */
JSBool generic_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	TSRMLS_FETCH();
	JSFunction				*class;
	JSString				*jclass_name;
	char					*class_name;
	zval					***params, *retval_ptr;
	zend_class_entry		*ce, **pce;
	zval					*cobj;

	php_jscontext_object	*intern;
	int						i;

	if (!JS_IsConstructing(cx))
	{
		return JS_FALSE;
	}

	/* first retrieve class name */
	class = JS_ValueToFunction(cx, ((argv)[-2]));
	jclass_name = JS_GetFunctionId(class);
	class_name = JS_GetStringBytes(jclass_name);

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);

	/* search for class entry */
	if (zend_hash_find(intern->ec_ht, class_name, strlen(class_name), (void**)&pce) == FAILURE) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0 TSRMLS_CC);
	}

	/* retrieve pointer to ce */
	ce = *pce;

	/* create object */
	MAKE_STD_ZVAL(cobj);

	if (ce->constructor)
	{
		zend_fcall_info			fci;
		zend_fcall_info_cache	fcc;

		if (!(ce->constructor->common.fn_flags & ZEND_ACC_PUBLIC)) {
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "Access to non-public constructor of class %s", class_name);
		}

		object_init_ex(cobj, ce);

		/* ready parameters */
		params = emalloc(argc * sizeof(zval**));
		for (i = 0; i < argc; i++)
		{
			zval *val;
			MAKE_STD_ZVAL(val);
			jsval_to_zval(val, cx, &argv[i]);
			SEPARATE_ARG_IF_REF(val);
			params[i] = &val;
		}

		fci.size			= sizeof(fci);
		fci.function_table	= EG(function_table);
		fci.function_name	= NULL;
		fci.symbol_table	= NULL;
		fci.object_ptr		= cobj;
		fci.retval_ptr_ptr	= &retval_ptr;
		fci.params			= params;
		fci.param_count		= argc;
		fci.no_separation	= 1;

		fcc.initialized		= 1;
		fcc.function_handler= ce->constructor;
		fcc.calling_scope	= EG(scope);
		fcc.called_scope	= Z_OBJCE_P(cobj);
		fcc.object_ptr		= cobj;

		if (zend_call_function(&fci, &fcc TSRMLS_CC) == FAILURE)
		{
			/* call ended, clean */
			for (i = 0; i < argc; i++)
			{
				zval *eval;
				eval = *params[i];
				zval_ptr_dtor(&eval);
				efree(eval);
			}
			if (retval_ptr) {
				zval_ptr_dtor(&retval_ptr);
			}
			efree(params);
			zval_ptr_dtor(&cobj);
			/* TODO: failed */
			*rval = JSVAL_VOID;
			return JS_FALSE;
		}

		/* call ended, clean */
		for (i = 0; i < argc; i++)
		{
			zval *eval;
			eval = *params[i];
			zval_ptr_dtor(&eval);
			efree(eval);
		}

		if (retval_ptr)
		{
			zval_ptr_dtor(&retval_ptr);
		}

		zval_to_jsval(cobj, cx, rval TSRMLS_CC);
	
		efree(params);
	}
	else
	{
		object_init_ex(cobj, ce);
		zval_to_jsval(cobj, cx, rval TSRMLS_CC);
	}

	zval_ptr_dtor(&cobj);

	return JS_TRUE;
}

JSBool JS_ResolvePHP(JSContext *cx, JSObject *obj, jsval id)
{
	/* always return true, as PHP doesn't use any resolver */
	return JS_TRUE;
}

JSBool JS_PropertySetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	TSRMLS_FETCH();
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	if (jsref != NULL)
	{
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval *val;

			str = JS_ValueToString(cx, id);
			prop_name = JS_GetStringBytes(str);

			MAKE_STD_ZVAL(val);
			jsval_to_zval(val, cx, vp);

			zend_update_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), val TSRMLS_CC);
			zval_ptr_dtor(&val);
		}
	}

	return JS_TRUE;
}

JSBool JS_PropertyGetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	TSRMLS_FETCH();
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	if (jsref != NULL) {
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval *val = NULL;
            JSBool has_property;

			str = JS_ValueToString(cx, id);
			prop_name = JS_GetStringBytes(str);

            has_property = JS_FALSE;
            if (JS_HasProperty(cx, obj, prop_name, &has_property) && (has_property == JS_TRUE)) {
                if (JS_LookupProperty(cx, obj, prop_name, vp) == JS_TRUE) {
                    return JS_TRUE;
                }
            }

			val = zend_read_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), 1 TSRMLS_CC);

			if (val != EG(uninitialized_zval_ptr)) {
				zval_add_ref(&val);
				zval_to_jsval(val, cx, vp TSRMLS_CC);
				zval_ptr_dtor(&val);
				return JS_TRUE;
			}
		}
	}
	
	return JS_TRUE;
}

/* This is called when a JSObject is destroyed by the GC or when the context
 * is detroyed */
void JS_FinalizePHP(JSContext *cx, JSObject *obj)
{
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	/* destroy ref object */
	if (jsref != NULL)
	{
		/* free the functions hash table */
		if (jsref->ht != NULL)
		{
			/* because we made a zval for each function callback, parse the whole
			 * hashtable back and free them */
			for(zend_hash_internal_pointer_reset(jsref->ht); zend_hash_has_more_elements(jsref->ht) == SUCCESS; zend_hash_move_forward(jsref->ht))
			{
				char *key;
				uint keylen;
				ulong idx;
				int type;
				php_callback *callback;

				/* retrieve current key */
				type = zend_hash_get_current_key_ex(jsref->ht, &key, &keylen, &idx, 0, NULL);
				if (zend_hash_get_current_data(jsref->ht, (void**)&callback) == FAILURE) {
					/* Should never actually fail
					* since the key is known to exist. */
					continue;
				}

				/* free the string used for the function name */
				zval_ptr_dtor(&callback->fci.function_name);
			}
			/* destroy hashtable */
			zend_hash_destroy(jsref->ht);
			FREE_HASHTABLE(jsref->ht);
		}

		/* remove reference to object and call ptr dtor */
		if (jsref->obj != NULL)
		{
			zval_ptr_dtor(&jsref->obj);
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
