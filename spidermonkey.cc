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

static inline php_jscontext_object* php_jscontext_fetch_object(zend_object *obj) {
    return (php_jscontext_object *)((char *)obj - XtOffsetOf(php_jscontext_object, zo));
}

static void php_jscontext_object_free_storage(zend_object *object)
{
	php_jscontext_object *intern = php_jscontext_fetch_object(object);

	// if a context is found ( which should be the case )
	// destroy it
	if (intern->ct != (JSContext*)NULL) {
		JS_LeaveCompartment(intern->ct, intern->cpt);
		JS_DestroyContext(intern->ct);
	}

	if (intern->ec_ht != NULL)
	{
		zend_hash_destroy(intern->ec_ht);
		FREE_HASHTABLE(intern->ec_ht);
	}

	zend_object_std_dtor(&intern->zo);
}

static zend_object* php_jscontext_object_new(zend_class_entry *ce)
{
	php_jscontext_object *intern;

	/* Allocate memory for it */
	intern = (php_jscontext_object *) ecalloc(1, sizeof(php_jscontext_object) + zend_object_properties_size(ce));

	/* if no runtime is found create one */
	if (SPIDERMONKEY_G(rt) == NULL)
	{
		SPIDERMONKEY_G(rt) = JS_NewRuntime(INI_INT("spidermonkey.gc_mem_threshold"), JS_NO_HELPER_THREADS);
	}

	/* exported classes hashlist */
	ALLOC_HASHTABLE(intern->ec_ht);
	zend_hash_init(intern->ec_ht, 20, NULL, NULL, 0);

	/* prepare hashtable for callback storage */
	intern->jsref = (php_jsobject_ref*)ecalloc(1, sizeof(php_jsobject_ref));
	/* create callback hashtable */
	ALLOC_HASHTABLE(intern->jsref->ht);
	zend_hash_init(intern->jsref->ht, 50, NULL, NULL, 0);

	/* the global object doesn't have any zval */
	intern->jsref->obj = NULL;

	intern->ct = JS_NewContext(SPIDERMONKEY_G(rt), 8192);
	PHPJS_START(intern->ct);
	JS_SetContextPrivate(intern->ct, intern);

	memset(&intern->script_class, 0, sizeof(intern->script_class));

	/* The script_class is a global object used by PHP to allow function register */
	intern->script_class.name			= "PHPClass";
	intern->script_class.flags			= JSCLASS_HAS_PRIVATE;

	/* Mandatory non-null function pointer members. */
	intern->script_class.addProperty	= JS_PropertyStub;
	intern->script_class.delProperty	= JS_DeletePropertyStub;
	intern->script_class.getProperty	= JS_PropertyGetterPHP;
	intern->script_class.setProperty	= JS_PropertySetterPHP;
	intern->script_class.resolve		= JS_ResolvePHP;
	intern->script_class.finalize		= JS_FinalizePHP;
	intern->script_class.enumerate		= JS_EnumerateStub;
	intern->script_class.convert		= JS_ConvertStub;

	memcpy(&intern->global_class, &intern->script_class, sizeof(intern->script_class));
	intern->global_class.name			= "PHPGlobalClass";
	intern->global_class.flags			= JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE;

	JSAutoRequest ar(intern->ct);

	/* says that our script runs in global scope */
	JS_SetOptions(intern->ct, JS_GetOptions(intern->ct) | JSOPTION_VAROBJFIX | JSOPTION_ASMJS);
	
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
	zend_object_std_init(&intern->zo, ce);
	object_properties_init(&intern->zo, ce);

	intern->zo.handlers = &jscontext_object_handlers;

	PHPJS_END(intern->ct);
	return &intern->zo;
}

static HashTable * get_gc(zval *object, zval **gc_data, int *gc_count)/*{{{*/
{
	*gc_data = NULL;
	*gc_count = 0;
	return zend_std_get_properties(object);
}
	
static void php_spidermonkey_init_globals(zend_spidermonkey_globals *hello_globals)
{
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
	ZEND_DECLARE_MODULE_GLOBALS(spidermonkey);
	ZEND_INIT_MODULE_GLOBALS(spidermonkey, php_spidermonkey_init_globals, NULL);

	SPIDERMONKEY_G(rt) = NULL;

	// here we set handlers to zero, meaning that we have no handlers set
	memcpy(&jscontext_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	jscontext_object_handlers.offset = XtOffsetOf(php_jscontext_object, zo);
	jscontext_object_handlers.get_gc = get_gc;
	jscontext_object_handlers.clone_obj = NULL;
	jscontext_object_handlers.free_obj = php_jscontext_object_free_storage;

	// init JSContext class
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSC_NAME, php_spidermonkey_jsc_functions);
	// this function will be called when the object is created by php
	ce.create_object = php_jscontext_object_new;
	// register class in PHP
	php_spidermonkey_jsc_entry = zend_register_internal_class(&ce);
	
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
		php_error_docref(NULL, E_WARNING, "JS runtime is already started, update of spidermonkey.gc_mem_threshold ignored.");
	}
}

/*  convert a given jsval in a context to a zval, for PHP access */
void _jsval_to_zval(zval *return_value, JSContext *ctx, JS::MutableHandle<JS::Value> rval, php_jsparent *parent)
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
				RETVAL_STRINGL(txt, strlen(txt));
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
			zend_throw_exception(zend_exception_get_default(TSRMLS_C), "Trying to reference global object", 0);
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
							zval fval;
							/* Call this function to convert a jsval to a zval */
							jsval_to_zval(&fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&value));
							/* Add property to our array */
							add_index_zval(return_value, i, &fval);
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
						zval fval;
						/* Call this function to convert a jsval to a zval */
						jsval_to_zval(&fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&item_val));
						/* Add property to our stdClass */
						add_assoc_zval(return_value, name, &fval);
					}
					JS_free(ctx, name);
				}
			}
		} else {
			if ((jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, &intern->script_class, NULL)) == NULL || jsref->obj == NULL)
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
								zval fval;

								/* Call this function to convert a jsval to a zval */
								_jsval_to_zval(&fval, ctx, JS::MutableHandleValue::fromMarkedLocation(&item_val), &jsthis);
								/* Add property to our stdClass */
								zend_update_property(NULL, return_value, name, strlen(name), &fval);
											
								zval_dtor(&fval);
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
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval)
{
	JSString				*jstr;
	JSObject				*jobj;
	zend_class_entry		*ce = NULL;
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;

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
		case IS_TRUE:
			*jval = BOOLEAN_TO_JSVAL(1);
			break;
		case IS_FALSE:
			*jval = BOOLEAN_TO_JSVAL(0);
			break;
		case IS_RESOURCE: {
			
			intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
			/* create JSObject */
			jobj = JS_NewObject(ctx, &intern->script_class, NULL, NULL);

			jsref = (php_jsobject_ref*)emalloc(sizeof(php_jsobject_ref));
			/* store pointer to object */
			// todo: is this necessary? this causes zend_object memory leak.
			// SEPARATE_ARG_IF_REF(val);
			jsref->ht = NULL;
			jsref->obj = val;
			/* auto define functions for stream */
			php_stream_from_zval_no_verify(stream, val);

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

		}
		case IS_OBJECT: {

			intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
			/* create JSObject */
			jobj = JS_NewObject(ctx, &intern->script_class, NULL, NULL);

			jsref = (php_jsobject_ref*)emalloc(sizeof(php_jsobject_ref));
			/* intern hashtable for function storage */
			ALLOC_HASHTABLE(jsref->ht);
			zend_hash_init(jsref->ht, 50, NULL, NULL, 0);

			// todo: is this necessary? this causes zend_object memory leak.
			// on FinalizePHP callback, ref count of val had changed to weird number.
			//SEPARATE_ARG_IF_REF(val);

			/* store pointer to object */
			jsref->obj = val;
			/* store pointer to HashTable */
			JS_SetPrivate(jobj, jsref);

			/* retrieve class entry */
			ce = Z_OBJCE_P(val);

			/* get function table */
			HashTable *func_ht = &ce->function_table;
			
			/* foreach functions */
			zend_string				*key;
			ulong num_key;
			zval*					zval_func;

			ZEND_HASH_FOREACH_KEY_VAL(func_ht, num_key, key, zval_func) {
				
				php_callback *callback = (php_callback*)ecalloc(1, sizeof(php_callback));
				
				zend_function* fptr = Z_FUNC_P(zval_func);

				zval function_name;
				ZVAL_STR(&function_name, fptr->common.function_name);
				
				/* then build the zend_fcall_info and cache */
				callback->fci.size = sizeof(callback->fci);
				callback->fci.function_name = function_name;
				callback->fci.retval = NULL;
				callback->fci.params = NULL;
				callback->fci.object = Z_OBJ_P(val);
				callback->fci.no_separation = 1;

				callback->fci_cache.initialized = 1;
				callback->fci_cache.function_handler = fptr;
				callback->fci_cache.calling_scope = ce;
				callback->fci_cache.object = Z_OBJ_P(val);

				/* store them */
				zend_hash_add_new_ptr(jsref->ht, fptr->common.function_name, callback);

				/* define the function */
				JS_DefineFunction(ctx, jobj, ZSTR_VAL(fptr->common.function_name), generic_call, 1, 0);
			} ZEND_HASH_FOREACH_END();

			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		}
		case IS_ARRAY: {
			/* retrieve the array hash table */
			HashTable *array_ht = HASH_OF(val);

			/* create JSObject */
			jobj = JS_NewArrayObject(ctx, 0, nullptr);

			// prevent GC
			JS_AddObjectRoot(ctx, &jobj);

			zend_string *string_key = NULL;
			zend_ulong num_key = 0;
			zval *z_value;

			ZEND_HASH_FOREACH_KEY_VAL(array_ht, num_key, string_key, z_value) {
				
				if (string_key) 
				{
					php_jsobject_set_property(ctx, jobj, ZSTR_VAL(string_key), z_value);
				}
				else 
				{
					jsval jarrval;
	
					/* first convert zval to jsval */
					zval_to_jsval(z_value, ctx, &jarrval);

					/* no ref behavior, just set a property */
					//JSBool res = JS_SetProperty(ctx, obj, property_name, &jval);
					//JSBool res = JS_SetElement(ctx, jobj, idx, &jarrval);
					JSBool res = JS_DefineElement(ctx, jobj, num_key, jarrval, nullptr, nullptr, 0);
				}
				
			} ZEND_HASH_FOREACH_END();
			

			*jval = OBJECT_TO_JSVAL(jobj);
			
			JS_RemoveObjectRoot(ctx, &jobj);
			break;
		}
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
