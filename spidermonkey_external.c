#include "php_spidermonkey.h"
#include "jsobj.h"

/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	/* throw error */
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), message, 0 TSRMLS_CC);
}

/* all function calls are mapped through this unique function */
JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString				*str;
	JSFunction				*func;
	JSString				*jfunc_name;
	char					*func_name;
	zval					***params, *retval_ptr;
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

	if (retval_ptr)
	{
		zval_to_jsval(retval_ptr, cx, rval);
		zval_ptr_dtor(&retval_ptr);
	}
	else
	{
		*rval = JSVAL_VOID;
	}
	
	efree(params);

	return JS_TRUE;
}

/* this native is used for class constructors */
JSBool generic_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString				*str;
	JSFunction				*class;
	JSString				*jclass_name;
	char					*class_name;
	zval					***params, *retval_ptr;
	zend_class_entry		*ce, **pce;
	zval					*cobj;

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	int						i;

	if (!JS_IsConstructing(cx))
	{
		// throw exception
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
			zval **val = emalloc(sizeof(zval*));
			MAKE_STD_ZVAL(*val);
			jsval_to_zval(*val, cx, &argv[i]);
			params[i] = val;
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
				zval **eval;
				eval = params[i];
				zval_ptr_dtor(eval);
				efree(eval);
			}
			if (retval_ptr) {
				zval_ptr_dtor(&retval_ptr);
			}
			efree(params);
			zval_ptr_dtor(&cobj);
			// TODO: failed
			*rval = JSVAL_VOID;
			return JS_FALSE;
		}

		/* call ended, clean */
		for (i = 0; i < argc; i++)
		{
			zval **eval;
			eval = params[i];
			zval_ptr_dtor(eval);
			efree(eval);
		}

		if (retval_ptr)
		{
			zval_ptr_dtor(&retval_ptr);
		}

		zval_to_jsval(cobj, cx, rval);
	
		efree(params);
	}
	else
	{
		object_init_ex(cobj, ce);
		zval_to_jsval(cobj, cx, rval);
	}

	zval_ptr_dtor(&cobj);

	return JS_TRUE;
}


JSBool JS_PropertyGetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	php_jsobject_ref		*jsref;
	php_jscontext_object	*intern;

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, &intern->script_class, NULL);

	if (jsref != NULL)
	{
		if (jsref->obj != NULL)
		{
			jsval js_propname;
			if (JS_IdToValue(cx, id, &js_propname) == JS_TRUE)
			{
				zval *val;
				JSString *str;
				jsval item_val;
				char *prop_name;

				str = JS_ValueToString(cx, js_propname);
				prop_name = JS_GetStringBytes(str);
				php_printf("reading property: %s\n", prop_name);

				if (zend_hash_find(Z_OBJPROP_P(jsref->obj), prop_name, sizeof(prop_name), (void**)&val) == FAILURE) {
					/* $rcvdclass->foo doesn't exist */
					return JS_TRUE;
				}

				zval_to_jsval(val, cx, vp);
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
		/* free the hash table */
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
			efree(jsref->ht);
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
