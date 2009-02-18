#include "php_spidermonkey.h"

static int le_jsruntime_descriptor;
static int le_jscontext_descriptor;

zend_module_entry spidermonkey_module_entry = {
	STANDARD_MODULE_HEADER,
	PHP_SPIDERMONKEY_EXTNAME,
	NULL, /* Functions */
	PHP_MINIT(spidermonkey), /* MINIT */
	PHP_MSHUTDOWN(spidermonkey), /* MSHUTDOWN */
	NULL, /* RINIT */
	NULL, /* RSHUTDOWN */
	PHP_MINFO(spidermonkey), /* MINFO */
	PHP_SPIDERMONKEY_EXTVER,
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SPIDERMONKEY
ZEND_GET_MODULE(spidermonkey)
#endif

/******************************
* JSRUNTIME STATIC METHODS
******************************/

static function_entry php_spidermonkey_jsr_functions[] = {
	PHP_ME(JSRuntime, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(JSRuntime, createContext, NULL, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};

static zend_object_handlers jsruntime_object_handlers;

/* called when the object is destroyed */
static void php_jsruntime_object_free_storage(void *object TSRMLS_DC)
{
	php_jsruntime_object *intern = (php_jsruntime_object *)object;

	/* there should always be a runtime there, so we free it */
	if (intern->rt != (JSRuntime *)NULL) {
		JS_DestroyRuntime(intern->rt);
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

/* called at object construct, we create the structure containing all data
 * needed by spidermonkey */
static zend_object_value php_jsruntime_object_new_ex(zend_class_entry *class_type, php_jsruntime_object **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_jsruntime_object *intern;

	/* Allocate memory for it */
	intern = (php_jsruntime_object *) emalloc(sizeof(php_jsruntime_object));
	memset(intern, 0, sizeof(php_jsruntime_object));

	if (ptr)
	{
		*ptr = intern;
	}

	intern->rt = JS_NewRuntime(PHP_JSRUNTIME_GC_MEMORY_THRESHOLD);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_jsruntime_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &jsruntime_object_handlers;

	return retval;
}

static zend_object_value php_jsruntime_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	return php_jsruntime_object_new_ex(class_type, NULL TSRMLS_CC);
}

/********************************
* JSCONTEXT STATIC CODE
********************************/

static function_entry php_spidermonkey_jsc_functions[] = {
	PHP_ME(JSContext, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(JSContext, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(JSContext, evaluateScript, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(JSContext, registerFunction, NULL, ZEND_ACC_PUBLIC)
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

	/* destroy hashtable */
	zend_hash_destroy(intern->ht);
	efree(intern->ht);

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(object);
}

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

	/* prepare hashtable for callback storage */
	intern->ht = (HashTable*)emalloc(sizeof(HashTable));
	zend_hash_init(intern->ht, 50, NULL, NULL, 0);

	/* create zend object */
	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_jscontext_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &jscontext_object_handlers;
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

	/*  CONSTANTS */
	
	/*  OPTIONS */
	REGISTER_LONG_CONSTANT("JSOPTION_ATLINE",				 JSOPTION_ATLINE,				 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_COMPILE_N_GO",			 JSOPTION_COMPILE_N_GO,			 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_DONT_REPORT_UNCAUGHT",	 JSOPTION_DONT_REPORT_UNCAUGHT,	 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_NATIVE_BRANCH_CALLBACK",JSOPTION_NATIVE_BRANCH_CALLBACK,CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_STRICT",				 JSOPTION_STRICT,				 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_VAROBJFIX",			 JSOPTION_VAROBJFIX,			 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_WERROR",			 	 JSOPTION_WERROR,				 CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSOPTION_XML",					 JSOPTION_XML,					 CONST_CS | CONST_PERSISTENT);

	/*  VERSIONS */
	REGISTER_LONG_CONSTANT("JSVERSION_1_0",	 JSVERSION_1_0,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_1",	 JSVERSION_1_1,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_2",	 JSVERSION_1_2,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_3",	 JSVERSION_1_3,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_4",	 JSVERSION_1_4,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_ECMA_3",  JSVERSION_ECMA_3,   CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_5",	 JSVERSION_1_5,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_6",	 JSVERSION_1_6,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_1_7",	 JSVERSION_1_7,	  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("JSVERSION_DEFAULT", JSVERSION_DEFAULT,  CONST_CS | CONST_PERSISTENT);

	/*  CLASS INIT */

	/* here we set handlers to zero, meaning that we have no handlers set */
	memcpy(&jsruntime_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	memcpy(&jscontext_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	zend_class_entry ce;

	/*  init JSRuntime class */
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSR_NAME, php_spidermonkey_jsr_functions);
	/* this function will be called when the object is created by php */
	ce.create_object = php_jsruntime_object_new;
	/* register class in PHP */
	php_spidermonkey_jsr_entry = zend_register_internal_class(&ce TSRMLS_CC);

	/*  init JSContext class */
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSC_NAME, php_spidermonkey_jsc_functions);
	ce.create_object = php_jscontext_object_new;
	php_spidermonkey_jsc_entry = zend_register_internal_class(&ce TSRMLS_CC);

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

/* convert a given jsval in a context to a zval, for PHP access */
void jsval_to_zval(zval *return_value, JSContext *ctx, jsval *jval)
{
	jsval   rval;

	memcpy(&rval, jval, sizeof(jsval));

	/* if it's a number or double, convert to double */
	if (JSVAL_IS_NUMBER(rval) || JSVAL_IS_DOUBLE(rval))
	{
		jsdouble d;
		if (JS_ValueToNumber(ctx, rval, &d) == JS_TRUE)
		{
			RETVAL_DOUBLE(d);
		}
		else
			RETVAL_FALSE;
	}
	else if (JSVAL_IS_INT(rval))
	{
		int d;
		d = JSVAL_TO_INT(rval);
		RETVAL_LONG(d);
	}
	else if (JSVAL_IS_STRING(rval))
	{
		JSString *str;
		/* first we convert the jsval to a JSString */
		str = JS_ValueToString(ctx, rval);
		if (str != NULL)
		{
			/* then we retrieve the pointer to the string */
			char *txt = JS_GetStringBytes(str);
			RETVAL_STRING(txt, strlen(txt));
		}
		else
		{
			RETVAL_FALSE;
		}
	}
	else if (JSVAL_IS_BOOLEAN(rval))
	{
		if (rval == JSVAL_TRUE)
		{
			RETVAL_TRUE;
		}
		else
		{
			RETVAL_FALSE;
		}
	}
	else if (JSVAL_IS_OBJECT(rval))
	{
		JSIdArray   *it;
		JSObject	*obj;
		jsid		*idp;
		int		 i;

		//  create stdClass
		object_init_ex(return_value, ZEND_STANDARD_CLASS_DEF_PTR);

		JS_ValueToObject(ctx, rval, &obj);

		//  then iterate on each property
		it = JS_Enumerate(ctx, obj);

		for (i = 0; i < it->length; i++)
		{
			jsval val;
			jsid id = it->vector[i];
			if (JS_IdToValue(ctx, id, &val) == JS_TRUE)
			{
				JSString *str;
				jsval item_val;

				str = JS_ValueToString(ctx, val);

				if (js_GetProperty(ctx, obj, id, &item_val) == JS_TRUE)
				{
					zval *fval;
					char *name;

					/* Retrieve property name */
					name = JS_GetStringBytes(str);

					MAKE_STD_ZVAL(fval);
					/* Call this function to convert a jsval to a zval */
					jsval_to_zval(fval, ctx, &item_val);
					/* Add property to our stdClass */
					zend_update_property(NULL, return_value, name, strlen(name), fval TSRMLS_CC);
					/* Destroy pointer to zval */
					zval_ptr_dtor(&fval);
				}
			}
		}

		JS_DestroyIdArray(ctx, it);
	}
	else if (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval))
	{
		RETVAL_NULL();
	}
	else /* something is wrong */
		RETVAL_FALSE;
}

/* convert a given jsval in a context to a zval, for PHP access */
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval)
{
	JSString				*jstr;
	JSObject				*jobj;
	HashTable				*ht, *iht;
	zend_class_entry		*ce = NULL;
	zend_function			*fptr;
	php_jscontext_object	*intern;

	if (val == NULL) {
		*jval = JSVAL_VOID;
		return;
	}

	switch(Z_TYPE_P(val))
	{
		case IS_DOUBLE:
			JS_NewNumberValue(ctx, Z_DVAL_P(val), jval);
			break;
		case IS_LONG:
			*jval = INT_TO_JSVAL(Z_LVAL_P(val));
			break;
		case IS_STRING:
			jstr = JS_NewStringCopyN(ctx, Z_STRVAL_P(val), Z_STRLEN_P(val));
			*jval = STRING_TO_JSVAL(jstr);
			break;
		case IS_BOOL:
			*jval = BOOLEAN_TO_JSVAL(Z_BVAL_P(val));
			break;
		case IS_OBJECT:
			intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
			/* create JSObject */
			jobj = JS_NewObject(ctx, &intern->script_class, NULL, NULL);
			/* retrieve class entry */
			ce = Z_OBJCE_P(val);
			/* get function table */
			ht = &ce->function_table;

			/* intern hashtable for function storage */
			iht = (HashTable*)emalloc(sizeof(HashTable));
			zend_hash_init(iht, 50, NULL, NULL, 0);
			/* store pointer to HashTable */
			JS_SetPrivate(ctx, jobj, iht);

			/* foreach item */
			for(zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
			{
				char					*key;
				uint					keylen;
				php_callback			cb;
				//jsval			jival;

				// retrieve current key
				zend_hash_get_current_key_ex(ht, &key, &keylen, 0, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&fptr) == FAILURE) {
					/* Should never actually fail
					* since the key is known to exist. */
					continue;
				}

				cb.fci.size = sizeof(cb.fci);
				cb.fci.function_table = NULL;
				cb.fci.function_name = NULL;
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

				zend_hash_add(iht, fptr->common.function_name, strlen(fptr->common.function_name), &cb, sizeof(cb), NULL);

				JS_DefineFunction(ctx, jobj, fptr->common.function_name, generic_call, 1, 0);

//				php_printf("%s\n", fptr->common.function_name);
			}
			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		case IS_ARRAY:
			/* retrieve the array hash table */
			ht = HASH_OF(val);

			/* create JSObject */
			jobj = JS_NewObject(ctx, NULL, NULL, NULL);

			/* foreach item */
			for(zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
			{
				char *key;
				uint keylen;
				ulong idx;
				int type;
				zval **ppzval, tmpcopy;
				jsval jival;
				char intIdx[25];

				// retrieve current key
				type = zend_hash_get_current_key_ex(ht, &key, &keylen, &idx, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
					/* Should never actually fail
					* since the key is known to exist. */
					continue;
				}

				/* Duplicate the zval so that
				 * the orignal's contents are not destroyed */
				tmpcopy = **ppzval;
				zval_copy_ctor(&tmpcopy);
				zval_to_jsval(&tmpcopy, ctx, &jival);
				if (type == HASH_KEY_IS_LONG)
				{
					sprintf(intIdx, "%d", idx);
					JS_SetProperty(ctx, jobj, intIdx, &jival);
				}
				else
				{
					JS_SetProperty(ctx, jobj, key, &jival);
				}

				zval_dtor(&tmpcopy);
			}
			
			*jval = OBJECT_TO_JSVAL(jobj);
			break;
		default:
			*jval = JSVAL_VOID;
			break;
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
