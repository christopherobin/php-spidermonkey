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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_spidermonkey.h"

#include <ext/standard/info.h>
#include <zend_exceptions.h>

//static int le_jscontext_descriptor;

ZEND_DECLARE_MODULE_GLOBALS(spidermonkey);

zend_module_entry spidermonkey_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_SPIDERMONKEY_EXTNAME,
	NULL, /* Functions */
	PHP_MINIT(spidermonkey), /* MINIT */
	PHP_MSHUTDOWN(spidermonkey), /* MSHUTDOWN */
	NULL, /* RINIT */
	PHP_RSHUTDOWN(spidermonkey), /* RSHUTDOWN */
	PHP_MINFO(spidermonkey), /* MINFO */
	PHP_SPIDERMONKEY_EXTVER,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SPIDERMONKEY
extern "C" {
ZEND_GET_MODULE(spidermonkey)
}
#endif

/* Out INI values */
PHP_INI_BEGIN()
// 8388608 is 8MB
PHP_INI_ENTRY("spidermonkey.gc_mem_threshold", PHP_JSRUNTIME_GC_MEMORY_THRESHOLD, PHP_INI_ALL, spidermonkey_ini_update)
PHP_INI_END()

/********************************
* JSCONTEXT STATIC CODE
********************************/

zend_function_entry php_spidermonkey_jsc_functions[] = {
	PHP_ME(JSContext, evaluateScript, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, registerFunction, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, registerClass, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, assign, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, setOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, toggleOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, getOptions, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, setVersion, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, getVersion, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, getVersionString, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers jscontext_object_handlers;

static void php_jscontext_object_free_storage(void *object TSRMLS_DC)
{
	php_jscontext_object *intern = (php_jscontext_object *)object;

	// if a context is found ( which should be the case )
	// destroy it
	if (intern->ct != (JSContext*)NULL) {
		PHPJS_START(intern->ct);
		JS_LeaveCompartment(intern->ct, intern->cpt);
		PHPJS_END(intern->ct);

		JS_DestroyContext(intern->ct);
	}

	if (intern->ec_ht != NULL)
	{
		zend_hash_destroy(intern->ec_ht);
		FREE_HASHTABLE(intern->ec_ht);
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

static JSClass phpjs_global_class = {
    "PHPGlobalClass",
	JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
};

static JSClass phpjs_php_class = {
    "PHPClass",
	JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
};

static zend_object_value php_jscontext_object_new_ex(zend_class_entry *class_type, php_jscontext_object **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_jscontext_object *intern;

	/* Allocate memory for it */
	intern = (php_jscontext_object *) emalloc(sizeof(php_jscontext_object));
	memset(intern, 0, sizeof(php_jscontext_object));

	if (ptr)
	{
		*ptr = intern;
	}

	/* if no runtime is found create one */
	if (SPIDERMONKEY_G(rt) == NULL)
	{
		SPIDERMONKEY_G(rt) = JS_NewRuntime(INI_INT("spidermonkey.gc_mem_threshold"), JS_NO_HELPER_THREADS);
	}

	/* exported classes hashlist */
	ALLOC_HASHTABLE(intern->ec_ht);
	zend_hash_init(intern->ec_ht, 20, NULL, NULL, 0);

	/* prepare hashtable for callback storage */
	intern->jsref = (php_jsobject_ref*)emalloc(sizeof(php_jsobject_ref));
	/* create callback hashtable */
	ALLOC_HASHTABLE(intern->jsref->ht);
	zend_hash_init(intern->jsref->ht, 50, NULL, NULL, 0);

	/* the global object doesn't have any zval */
	intern->jsref->obj = NULL;

	intern->ct = JS_NewContext(SPIDERMONKEY_G(rt), 8192);
	PHPJS_START(intern->ct);
	JS_SetContextPrivate(intern->ct, intern);

	intern->script_class = phpjs_php_class;
	intern->global_class = phpjs_global_class;

	JSAutoRequest ar(intern->ct);

	/* says that our script runs in global scope */
	JS_SetOptions(intern->ct, JS_GetOptions(intern->ct) | JSOPTION_VAROBJFIX | JSOPTION_UNROOTED_GLOBAL);

	/* set the error callback */
	JS_SetErrorReporter(intern->ct, reportError);

	/* create global object for execution */
	intern->obj = JS_NewGlobalObject(intern->ct, &intern->global_class, nullptr);

	JS_SetGlobalObject(intern->ct, intern->obj);
	intern->cpt = JS_EnterCompartment(intern->ct, intern->obj);

	/* initialize standard JS classes */
	JS_InitStandardClasses(intern->ct, intern->obj);

	/* store pointer to HashTable */
	JS_SetPrivate(intern->obj, intern->jsref);

	/* create zend object */
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_jscontext_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &jscontext_object_handlers;
	PHPJS_END(intern->ct);
	return retval;
}

static zend_object_value php_jscontext_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	return php_jscontext_object_new_ex(class_type, NULL TSRMLS_CC);
}

/**
* Extension code
*/
PHP_MINIT_FUNCTION(spidermonkey)
{
	zend_class_entry ce;

	// INI VALUES
	REGISTER_INI_ENTRIES();

	// CONSTANTS

	// OPTIONS
	REGISTER_LONG_CONSTANT("JSOPTION_COMPILE_N_GO",			 JSOPTION_COMPILE_N_GO,			 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_DONT_REPORT_UNCAUGHT",	 JSOPTION_DONT_REPORT_UNCAUGHT,	 CONST_CS | CONST_PERSISTENT);
#ifdef JSOPTION_NATIVE_BRANCH_CALLBACK /* Fix for version 1.9 */
	REGISTER_LONG_CONSTANT("JSOPTION_NATIVE_BRANCH_CALLBACK",JSOPTION_NATIVE_BRANCH_CALLBACK,CONST_CS | CONST_PERSISTENT);
#endif
	REGISTER_LONG_CONSTANT("JSOPTION_VAROBJFIX",			 JSOPTION_VAROBJFIX,			 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_WERROR",			 	 JSOPTION_WERROR,				 CONST_CS | CONST_PERSISTENT);

	/*  VERSIONS */
	REGISTER_LONG_CONSTANT("JSVERSION_ECMA_3",  JSVERSION_ECMA_3,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_6",	 JSVERSION_1_6,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_7",	 JSVERSION_1_7,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_DEFAULT", JSVERSION_DEFAULT,  CONST_CS | CONST_PERSISTENT);

	// CLASS INIT
#ifdef ZTS
	//ts_allocate_id(&spidermonkey_globals_id, sizeof(spidermonkey_globals), NULL, NULL);
	ZEND_INIT_MODULE_GLOBALS(spidermonkey, NULL, NULL);
#endif

	SPIDERMONKEY_G(rt) = NULL;

	// here we set handlers to zero, meaning that we have no handlers set
	memcpy(&jscontext_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	// init JSContext class
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSC_NAME, php_spidermonkey_jsc_functions);
	// this function will be called when the object is created by php
	ce.create_object = php_jscontext_object_new;
	// register class in PHP
	php_spidermonkey_jsc_entry = zend_register_internal_class(&ce TSRMLS_CC);

	return SUCCESS;
}

/* I was not doing this before, which mean that in Apache, all created JSRuntime
 * were only freed when the server was shutdown */
PHP_RSHUTDOWN_FUNCTION(spidermonkey)
{
	if (SPIDERMONKEY_G(rt) != NULL) {
		JS_DestroyRuntime(SPIDERMONKEY_G(rt));
		SPIDERMONKEY_G(rt) = NULL; // DestroyRuntime doesn't change the pointer value, set it to null to avoid bugs
	}
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(spidermonkey)
{
	/*  free everything JS* could have allocated */
	JS_ShutDown();

	return SUCCESS;
}

PHP_MINFO_FUNCTION(spidermonkey)
{
	php_info_print_table_start();
	php_info_print_table_row(2, PHP_SPIDERMONKEY_MINFO_NAME, "enabled");
	php_info_print_table_row(2, "Version", PHP_SPIDERMONKEY_EXTVER);
	php_info_print_table_row(2, "LibJS Version", JS_GetImplementationVersion());
	php_info_print_table_end();
}

PHP_INI_MH(spidermonkey_ini_update) {
	// if the runtime is already here, emit a warning
	if (SPIDERMONKEY_G(rt) != NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "JS runtime is already started, update of spidermonkey.gc_mem_threshold ignored.");
	}
}

/*  convert a given jsval in a context to a zval, for PHP access */
void _jsval_to_zval(zval *return_value, JSContext *ctx, JS::MutableHandle<JS::Value> rval, php_jsparent *parent TSRMLS_DC)
{
	PHPJS_START(ctx);

	if (rval.isNullOrUndefined())
	{
		RETVAL_NULL();
	}
	else if (rval.isDouble())
	{
		RETVAL_DOUBLE(rval.toDouble());
	}
	else if (rval.isInt32())
	{
		RETVAL_LONG(rval.toInt32());
	}
	else if (rval.isString())
	{
		JSString *str;
		int len;
		/* first we convert the jsval to a JSString */
		str = rval.toString();
		if (str != NULL)
		{
			/* check string length and return an empty string if the
			   js string is empty (bug 16876) */
			if ((len = JS_GetStringLength(str)) > 0) {
				/* then we retrieve the pointer to the string */
				char *txt = JS_EncodeString(ctx, str);
				RETVAL_STRINGL(txt, strlen(txt), 1);
				JS_free(ctx, txt);
			}
			else
			{
				RETVAL_EMPTY_STRING();
			}
		}
		else
		{
			RETVAL_FALSE;
		}
	}
	else if (rval.isBoolean())
	{
		if (rval.isTrue())
		{
			RETVAL_TRUE;
		}
		else
		{
			RETVAL_FALSE;
		}
	}
	else if (rval.isObject())
	{
		JSObject				*it;
		JSObject				*obj = NULL;
		int						i;
		php_jscontext_object	*intern;
		php_jsobject_ref		*jsref;
		zval					*zobj;
		php_jsparent			jsthis;

		obj = rval.toObjectOrNull();

		if (obj == nullptr) {
			PHPJS_END(ctx);
			RETURN_NULL();
		}

		/* your shouldn't be able to reference the global object */
		if (obj == JS_GetGlobalForScopeChain(ctx)) {
			PHPJS_END(ctx);
			zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Trying to reference global object", 0 TSRMLS_CC);
			return;
		}

		intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);

		if (JS_IsArrayObject(ctx, obj)) {
			array_init(return_value);

			uint32_t len = 0;
			if (JS_GetArrayLength(ctx, obj, &len) == JS_TRUE) {
				if (len > 0) {
					for (uint32_t i = 0; i < len; i++) {
						jsval value;

						if (JS_LookupElement(ctx, obj, i, &value) == JS_TRUE) {
							zval *fval;

							/* alloc memory for this zval */
							MAKE_STD_ZVAL(fval);
							/* Call this function to convert a jsval to a zval */
							jsval_to_zval(fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&value));
							/* Add property to our array */
							add_index_zval(return_value, i, fval);
						}
					}
				}
			}

			/* then iterate on each property */
			it = JS_NewPropertyIterator(ctx, obj);

			jsid id;
			while (JS_NextProperty(ctx, it, &id) == JS_TRUE)
			{
				jsval val;

				if (JSID_IS_VOID(id)) {
					break;
				}

				if (JS_IdToValue(ctx, id, &val) == JS_TRUE)
				{
					JSString *str;
					jsval item_val;
					char *name;

					str = JS_ValueToString(ctx, val);

					/* Retrieve property name */
					name = JS_EncodeString(ctx, str);

					/* Try to read property */
					if (JS_GetProperty(ctx, obj, name, &item_val) == JS_TRUE)
					{
						zval *fval;

						/* alloc memory for this zval */
						MAKE_STD_ZVAL(fval);
						/* Call this function to convert a jsval to a zval */
						jsval_to_zval(fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&item_val));
						/* Add property to our stdClass */
						add_assoc_zval(return_value, name, fval);
					}
					JS_free(ctx, name);
				}
			}
		} else {
			if ((jsref = (php_jsobject_ref*)JS_GetPrivate(obj)) == NULL || jsref->obj == NULL)
			{
				zobj = NULL;
				while (parent != NULL) {
					if (parent->obj == obj) {
						zobj = parent->zobj;
						break;
					}
					parent = parent->parent;
				}

				if (zobj == NULL)
				{
					/* create stdClass */
					object_init_ex(return_value, ZEND_STANDARD_CLASS_DEF_PTR);
					/* store value */
					jsthis.obj     = obj;
					jsthis.zobj    = return_value;
					jsthis.parent  = parent;

					/* then iterate on each property */
					it = JS_NewPropertyIterator(ctx, obj);

					jsid id;
					while (JS_NextProperty(ctx, it, &id) == JS_TRUE)
					{
						jsval val;

						if (JSID_IS_VOID(id)) {
							break;
						}

						if (JS_IdToValue(ctx, id, &val) == JS_TRUE)
						{
							JSString *str;
							jsval item_val;
							char *name;

							str = JS_ValueToString(ctx, val);

							/* Retrieve property name */
							name = JS_EncodeString(ctx, str);

							/* Try to read property */
							if (JS_GetProperty(ctx, obj, name, &item_val) == JS_TRUE)
							{
								zval *fval;

								/* alloc memory for this zval */
								MAKE_STD_ZVAL(fval);
								/* Call this function to convert a jsval to a zval */
								_jsval_to_zval(fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&item_val), &jsthis TSRMLS_CC);
								/* Add property to our stdClass */
								zend_update_property(NULL, return_value, name, strlen(name), fval TSRMLS_CC);
								/* Destroy pointer to zval */
								zval_ptr_dtor(&fval);
							}
							JS_free(ctx, name);
						}
					}
				} else {
					RETVAL_ZVAL(zobj, 1, NULL);
				}
			}
			else
			{
				RETVAL_ZVAL(jsref->obj, 1, NULL);
			}
		}
	}
	else /* something is wrong */
		RETVAL_FALSE;

	PHPJS_END(ctx);
}

/* convert a given jsval in a context to a zval, for PHP access */
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval TSRMLS_DC)
{
	JSString				*jstr;
	JSObject				*jobj;
	HashTable				*ht;
	zend_class_entry		*ce = NULL;
	zend_function			*fptr;
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream;

	PHPJS_START(ctx);

	if (val == NULL) {
		*jval = JSVAL_NULL;
		PHPJS_END(ctx);
		return;
	}

	switch(Z_TYPE_P(val))
	{
		case IS_LONG:
			jval->setNumber((uint32_t)Z_LVAL_P(val));
			break;
		case IS_DOUBLE:
			jval->setNumber(Z_DVAL_P(val));
			break;
		case IS_STRING:
			jstr = JS_NewStringCopyN(ctx, Z_STRVAL_P(val), Z_STRLEN_P(val));
			jval->setString(jstr);
			break;
		case IS_BOOL:
			*jval = BOOLEAN_TO_JSVAL(Z_BVAL_P(val));
			break;
		case IS_RESOURCE:
			intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
			/* create JSObject */
			jobj = JS_NewObject(ctx, &intern->script_class, NULL, NULL);

			jsref = (php_jsobject_ref*)emalloc(sizeof(php_jsobject_ref));
			/* store pointer to object */
			SEPARATE_ARG_IF_REF(val);
			jsref->ht = NULL;
			jsref->obj = val;
			/* auto define functions for stream */
			php_stream *stream;
			php_stream_from_zval_no_verify(stream, &val);

			if (stream != NULL) {
				/* set a bunch of constants */
				jsval js_const;
				js_const = INT_TO_JSVAL(SEEK_SET);
				JS_SetProperty(ctx, jobj, "SEEK_SET", &js_const);
				js_const = INT_TO_JSVAL(SEEK_CUR);
				JS_SetProperty(ctx, jobj, "SEEK_CUR", &js_const);
				js_const = INT_TO_JSVAL(SEEK_END);
				JS_SetProperty(ctx, jobj, "SEEK_END", &js_const);
				/* set stream functions */
				JS_DefineFunction(ctx, jobj, "read", js_stream_read, 1, 0);
				JS_DefineFunction(ctx, jobj, "getline", js_stream_getline, 1, 0);
				JS_DefineFunction(ctx, jobj, "getl", js_stream_getline, 1, 0);
				JS_DefineFunction(ctx, jobj, "seek", js_stream_seek, 1, 0);
				JS_DefineFunction(ctx, jobj, "write", js_stream_write, 1, 0);
				JS_DefineFunction(ctx, jobj, "tell", js_stream_tell, 1, 0);
			}

			/* store pointer to HashTable */
			JS_SetPrivate(jobj, jsref);

			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		case IS_OBJECT:
			intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
			/* create JSObject */
			jobj = JS_NewObject(ctx, &intern->script_class, NULL, NULL);

			jsref = (php_jsobject_ref*)emalloc(sizeof(php_jsobject_ref));
			/* intern hashtable for function storage */
			ALLOC_HASHTABLE(jsref->ht);
			zend_hash_init(jsref->ht, 50, NULL, NULL, 0);

			SEPARATE_ARG_IF_REF(val);

			/* store pointer to object */
			jsref->obj = val;
			/* store pointer to HashTable */
			JS_SetPrivate(jobj, jsref);

			/* retrieve class entry */
			ce = Z_OBJCE_P(val);
			/* get function table */
			ht = &ce->function_table;
			/* foreach functions */
			for(zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
			{
				char					*key;
				uint					keylen;
				php_callback			cb;
				zval					*z_fname;

				/* retrieve current key */
				zend_hash_get_current_key_ex(ht, &key, &keylen, 0, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&fptr) == FAILURE) {
					/* Should never actually fail
					 * since the key is known to exist. */
					continue;
				}

				/* store the function name as a zval */
				MAKE_STD_ZVAL(z_fname);
				ZVAL_STRING(z_fname, fptr->common.function_name, 1);

				/* then build the zend_fcall_info and cache */
				cb.fci.size = sizeof(cb.fci);
				cb.fci.function_table = &ce->function_table;
				cb.fci.function_name = z_fname;
				cb.fci.symbol_table = NULL;
				cb.fci.object_ptr = val;
				cb.fci.retval_ptr_ptr = NULL;
				cb.fci.param_count = fptr->common.num_args;
				cb.fci.params = NULL;
				cb.fci.no_separation = 1;

				cb.fci_cache.initialized = 1;
				cb.fci_cache.function_handler = fptr;
				cb.fci_cache.calling_scope = ce;
				cb.fci_cache.object_ptr = val;

				/* store them */
				zend_hash_add(jsref->ht, fptr->common.function_name, strlen(fptr->common.function_name), &cb, sizeof(cb), NULL);

				/* define the function */
				JS_DefineFunction(ctx, jobj, fptr->common.function_name, generic_call, 1, 0);
			}
			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		case IS_ARRAY:
			/* retrieve the array hash table */
			ht = HASH_OF(val);

			/* create JSObject */
			jobj = JS_NewArrayObject(ctx, 0, nullptr);

			// prevent GC
			JS_AddObjectRoot(ctx, &jobj);

			/* foreach item */
			for(zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
			{
				char *key;
				uint keylen;
				ulong idx;
				int type;
				zval **ppzval;
				char intIdx[25];

				/* retrieve current key */
				type = zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
					/* Should never actually fail
					 * since the key is known to exist. */
					continue;
				}

				if (type == HASH_KEY_IS_LONG)
				{
					//sprintf(intIdx, "%ld", idx);
					//php_jsobject_set_property(ctx, jobj, intIdx, *ppzval TSRMLS_CC);
					jsval jarrval;

					/* first convert zval to jsval */
					zval_to_jsval(*ppzval, ctx, &jarrval TSRMLS_CC);

					/* no ref behavior, just set a property */
					//JSBool res = JS_SetProperty(ctx, obj, property_name, &jval);
					//JSBool res = JS_SetElement(ctx, jobj, idx, &jarrval);
					JSBool res = JS_DefineElement(ctx, jobj, idx, jarrval, nullptr, nullptr, 0);

				}
				else
				{
					php_jsobject_set_property(ctx, jobj, key, *ppzval TSRMLS_CC);
				}
			}

			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		case IS_NULL:
			*jval = JSVAL_NULL;
			break;
		default:
			*jval = JSVAL_VOID;
			break;
	}
	PHPJS_END(ctx);
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
