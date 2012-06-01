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
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	/* throw error */
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), (char *) message, 0 TSRMLS_CC);
}

/* this function set a property on an object */
void php_jsobject_set_property(JSContext *ctx, JSObject *obj, char *property_name, zval *val TSRMLS_DC)
{
	jsval jval;
	
	PHPJS_START(ctx);

	/* first convert zval to jsval */
	zval_to_jsval(val, ctx, &jval TSRMLS_CC);

	/* no ref behavior, just set a property */
	JS_SetProperty(ctx, obj, property_name, &jval);
	
	PHPJS_END(ctx);
}

/* all function calls are mapped through this unique function */
#if JS_VERSION < 185
JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool generic_call(JSContext *cx, uintN argc, jsval *vp)
#endif
{
#ifdef ZTS
	TSRMLS_FETCH(); /* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
#endif
	JSFunction				*func;
	JSString				*jfunc_name;
	JSClass					*class;
	char					*func_name;
	zval					***params, *retval_ptr = NULL;
	php_callback			*callback;
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	int						i;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	/* first retrieve function name */
	func = JS_ValueToFunction(cx, ((argv)[-2]));
	jfunc_name = JS_GetFunctionId(func);
#if JS_VERSION < 185
	func_name = JS_GetStringBytes(jfunc_name);
#else
	/* because version 1.8.5 supports unicode, we must encode strings */
	func_name = JS_EncodeString(cx, jfunc_name);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;
	
	if (obj == intern->obj) {
		class =&intern->global_class;
	}
	
	if ((jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL)) == 0)
	{
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function table", 0 TSRMLS_CC);
	}

	/* search for function callback */
	if (zend_hash_find(jsref->ht, func_name, strlen(func_name), (void**)&callback) == FAILURE) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0 TSRMLS_CC);
	}

	/* free function name */
#if JS_VERSION >= 185
	JS_free(cx, func_name);
#endif

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
#if JS_VERSION < 185
JSBool generic_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool generic_constructor(JSContext *cx, uintN argc, jsval *vp)
#endif
{
#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
#endif
	JSFunction				*class;
	JSString				*jclass_name;
	char					*class_name;
	zval					***params, *retval_ptr;
	zend_class_entry		*ce, **pce;
	zval					*cobj;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	php_jscontext_object	*intern;
	int						i;

#if JS_VERSION < 185
	if (!JS_IsConstructing(cx))
	{
		return JS_FALSE;
	}
#endif

	/* first retrieve class name */
	class = JS_ValueToFunction(cx, ((argv)[-2]));
	jclass_name = JS_GetFunctionId(class);
#if JS_VERSION < 185
	class_name = JS_GetStringBytes(jclass_name);
#else
	/* because version 1.8.5 supports unicode, we must encode strings */
	class_name = JS_EncodeString(cx, jclass_name);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);

	/* search for class entry */
	if (zend_hash_find(intern->ec_ht, class_name, strlen(class_name), (void**)&pce) == FAILURE) {
		zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Failed to retrieve function callback", 0 TSRMLS_CC);
	}
	
	/* free class name */
#if JS_VERSION >= 185
	JS_free(cx, class_name);
#endif

	/* retrieve pointer to ce */
	ce = *pce;

	/* create object */
	MAKE_STD_ZVAL(cobj);

	if (ce->constructor)
	{
		zend_fcall_info			fci;
		zend_fcall_info_cache	fcc;

		if (!(ce->constructor->common.fn_flags & ZEND_ACC_PUBLIC)) {
#if JS_VERSION < 185
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "Access to non-public constructor of class %s", class_name);
#else
			/* we can't use spidermonkey's class_name pointer because it's not managed by it */
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "Access to non-public constructor");
#endif
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
#if JS_VERSION < 185
JSBool JS_ResolvePHP(JSContext *cx, JSObject *obj, jsval id)
#else
JSBool JS_ResolvePHP(JSContext *cx, JSObject *obj, jsid id)
#endif
{
	/* always return true, as PHP doesn't use any resolver */
	return JS_TRUE;
}

#if JS_VERSION < 185
JSBool JS_PropertySetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
JSBool JS_PropertySetterPHP(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;
	JSClass					*class;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL)
	{
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval *val;

#if JS_VERSION < 185
			str = JS_ValueToString(cx, id);
			prop_name = JS_GetStringBytes(str);
#else
			/* 1.8.5 uses reals jsid for id, we need to convert it */
			jsval rid;
			JS_IdToValue(cx, id, &rid);
			str = JS_ValueToString(cx, rid);
			/* because version 1.8.5 supports unicode, we must encode strings */
			prop_name = JS_EncodeString(cx, str);
#endif

			MAKE_STD_ZVAL(val);
			jsval_to_zval(val, cx, vp);

			zend_update_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), val TSRMLS_CC);
			zval_ptr_dtor(&val);

			/* free prop name */
#if JS_VERSION >= 185
			JS_free(cx, prop_name);
#endif
		}
	}

	return JS_TRUE;
}

#if JS_VERSION < 185
JSBool JS_PropertyGetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
#else
JSBool JS_PropertyGetterPHP(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;
	JSClass					*class;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;
	
	if (obj == intern->obj) {
		class =&intern->global_class;
	}
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	if (jsref != NULL) {
		if (jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_OBJECT) {
			JSString *str;
			char *prop_name;
			zval *val = NULL;
            JSBool has_property;

#if JS_VERSION < 185
			str = JS_ValueToString(cx, id);
			prop_name = JS_GetStringBytes(str);
#else
			/* 1.8.5 uses reals jsid for id, we need to convert it */
			jsval rid;
			JS_IdToValue(cx, id, &rid);
			str = JS_ValueToString(cx, rid);
			/* because version 1.8.5 supports unicode, we must encode strings */
			prop_name = JS_EncodeString(cx, str);
#endif

            has_property = JS_FALSE;
            if (JS_HasProperty(cx, obj, prop_name, &has_property) && (has_property == JS_TRUE)) {
                if (JS_LookupProperty(cx, obj, prop_name, vp) == JS_TRUE) {
					/* free prop name */
#if JS_VERSION >= 185
					JS_free(cx, prop_name);
#endif
                    return JS_TRUE;
                }
            }

			val = zend_read_property(Z_OBJCE_P(jsref->obj), jsref->obj, prop_name, strlen(prop_name), 1 TSRMLS_CC);

			/* free prop name */
#if JS_VERSION >= 185
			JS_free(cx, prop_name);
#endif

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
