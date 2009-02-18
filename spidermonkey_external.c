#include "php_spidermonkey.h"

/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	/* throw error */
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), message, 0 TSRMLS_CC);
}

JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString				*str;
	JSFunction				*func;
	JSString				*jfunc_name;
	char					*func_name;
	zval					***params, *retval_ptr;
	php_callback			*callback;
	php_jscontext_object	*intern;
//	HashTable				*ht;
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

//	php_printf("Size: %d\nFunction table: %p\nFunction name: %s\n", callback->fci.size, callback->fci.function_table, callback->fci.function_name);

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

void JS_FinalizePHP(JSContext *cx, JSObject *obj)
{
	php_jsobject_ref        *jsref;
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

				// retrieve current key
				type = zend_hash_get_current_key_ex(jsref->ht, &key, &keylen, &idx, 0, NULL);
				if (zend_hash_get_current_data(jsref->ht, (void**)&callback) == FAILURE) {
					/* Should never actually fail
					* since the key is known to exist. */
					continue;
				}

				// free the string used for the function name
				zval_ptr_dtor(&callback->fci.function_name);
			}
			/* destroy hashtable */
			zend_hash_destroy(jsref->ht);
			efree(jsref->ht);
		}

		/* remove reference to object and call ptr dtor */
		if (jsref->obj != NULL)
		{
			Z_DELREF_P(jsref->obj);
			zval_ptr_dtor(&jsref->obj);
		}

		/* then destroy JSRef */
		efree(jsref);
	}
}
