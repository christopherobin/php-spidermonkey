#include "php_spidermonkey.h"

static int le_jsruntime_descriptor;
static int le_jscontext_descriptor;
static int le_jsobject_descriptor;

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

static void php_jsruntime_object_free_storage(void *object TSRMLS_DC)
{
	php_jsruntime_object *intern = (php_jsruntime_object *)object;

	if (!intern) {
		return;
	}

	if (intern->rt != (JSRuntime *)NULL) {
	    JS_DestroyRuntime(intern->rt);
	    //intern->rt = NULL;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static zend_object_value php_jsruntime_object_new_ex(zend_class_entry *class_type, php_jsruntime_object **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_jsruntime_object *intern;

	/* Allocate memory for it */
	intern = (php_jsruntime_object *) emalloc(sizeof(php_jsruntime_object));
	memset(&intern->zo, 0, sizeof(zend_object));
	
	if (ptr) {
		*ptr = intern;
	}

    intern->rt = JS_NewRuntime(PHP_JSRUNTIME_GC_MEMORY_THRESHOLD);

	//ALLOC_HASHTABLE(intern->zo.properties);

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

// js context contructor need 1 arg, a JSRuntime class
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 3
static
#endif
    ZEND_BEGIN_ARG_INFO(php_spidermonkey_jsc_arginfo, 0)
        ZEND_ARG_OBJ_INFO(1, "obj", JSRuntime, 0)
    ZEND_END_ARG_INFO()

static function_entry php_spidermonkey_jsc_functions[] = {
    PHP_ME(JSContext, __construct, php_spidermonkey_jsc_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(JSContext, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
    PHP_ME(JSContext, createObject, NULL, ZEND_ACC_PUBLIC)
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

	if (!intern) {
		return;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static zend_object_value php_jscontext_object_new_ex(zend_class_entry *class_type, php_jscontext_object **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_jscontext_object *intern;

	/* Allocate memory for it */
	intern = (php_jscontext_object *) emalloc(sizeof(php_jscontext_object));
	memset(&intern->zo, 0, sizeof(zend_object));
	
	if (ptr) {
		*ptr = intern;
	}

	//ALLOC_HASHTABLE(intern->zo.properties);

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


/********************************
* JSOBJECT STATIC CODE
********************************/

// js context contructor need 1 arg, a JSContext class
#if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 3
static
#endif
    ZEND_BEGIN_ARG_INFO(php_spidermonkey_jso_arginfo, 0)
        ZEND_ARG_OBJ_INFO(1, "obj", JSContext, 0)
    ZEND_END_ARG_INFO()

static function_entry php_spidermonkey_jso_functions[] = {
    PHP_ME(JSObject, __construct, php_spidermonkey_jso_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
    PHP_ME(JSObject, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
    PHP_ME(JSObject, evaluateScript, NULL, ZEND_ACC_PUBLIC)
    { NULL, NULL, NULL }
};

static zend_object_handlers jsobject_object_handlers;

static void php_jsobject_object_free_storage(void *object TSRMLS_DC)
{
	php_jsobject_object *intern = (php_jsobject_object *)object;

	if (!intern) {
		return;
	}

	zend_object_std_dtor(&intern->zo TSRMLS_CC);
	efree(intern);
}

static zend_object_value php_jsobject_object_new_ex(zend_class_entry *class_type, php_jsobject_object **ptr TSRMLS_DC)
{
	zval *tmp;
	zend_object_value retval;
	php_jsobject_object *intern;

	/* Allocate memory for it */
	intern = (php_jsobject_object *) emalloc(sizeof(php_jsobject_object));
	memset(&intern->zo, 0, sizeof(zend_object));
	
	if (ptr) {
		*ptr = intern;
	}

	//ALLOC_HASHTABLE(intern->zo.properties);

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &tmp, sizeof(zval *));

	retval.handle = zend_objects_store_put(intern, NULL, (zend_objects_free_object_storage_t) php_jsobject_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &jsobject_object_handlers;
	return retval;
}

static zend_object_value php_jsobject_object_new(zend_class_entry *class_type TSRMLS_DC)
{
    return php_jsobject_object_new_ex(class_type, NULL TSRMLS_CC);
}

/**
* Extension code
*/
PHP_MINIT_FUNCTION(spidermonkey)
{

    // CONSTANTS
    
    // OPTIONS
    REGISTER_LONG_CONSTANT("JSOPTION_ATLINE",                   JSOPTION_ATLINE,                    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_COMPILE_N_GO",             JSOPTION_COMPILE_N_GO,              CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_DONT_REPORT_UNCAUGHT",     JSOPTION_DONT_REPORT_UNCAUGHT,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_NATIVE_BRANCH_CALLBACK",   JSOPTION_NATIVE_BRANCH_CALLBACK,    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_STRICT",                   JSOPTION_STRICT,                    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_VAROBJFIX",                JSOPTION_VAROBJFIX,                 CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_WERROR",                   JSOPTION_WERROR,                    CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSOPTION_XML",                      JSOPTION_XML,                       CONST_CS | CONST_PERSISTENT);

    // VERSIONS
    REGISTER_LONG_CONSTANT("JSVERSION_1_0",     JSVERSION_1_0,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_1",     JSVERSION_1_1,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_2",     JSVERSION_1_2,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_3",     JSVERSION_1_3,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_4",     JSVERSION_1_4,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_ECMA_3",  JSVERSION_ECMA_3,   CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_5",     JSVERSION_1_5,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_6",     JSVERSION_1_6,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_1_7",     JSVERSION_1_7,      CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("JSVERSION_DEFAULT", JSVERSION_DEFAULT,  CONST_CS | CONST_PERSISTENT);

    // CLASS INIT

    memcpy(&jsruntime_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    memcpy(&jscontext_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    memcpy(&jsobject_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

    zend_class_entry ce;

    // init JSRuntime class
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSR_NAME, php_spidermonkey_jsr_functions);
	ce.create_object = php_jsruntime_object_new;
	php_spidermonkey_jsr_entry = zend_register_internal_class(&ce TSRMLS_CC);

    // init JSContext class
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSC_NAME, php_spidermonkey_jsc_functions);
	ce.create_object = php_jscontext_object_new;
	php_spidermonkey_jsc_entry = zend_register_internal_class(&ce TSRMLS_CC);

    // init JSObject class
	INIT_CLASS_ENTRY(ce, PHP_SPIDERMONKEY_JSO_NAME, php_spidermonkey_jso_functions);
	ce.create_object = php_jsobject_object_new;
	php_spidermonkey_jso_entry = zend_register_internal_class(&ce TSRMLS_CC);

    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(spidermonkey)
{
    // free everything JS* could have allocated
    JS_ShutDown();
    
    return SUCCESS;
}

PHP_MINFO_FUNCTION(spidermonkey)
{
    php_info_print_table_start();
    php_info_print_table_row(2, PHP_SPIDERMONKEY_MINFO_NAME, "enabled");
    php_info_print_table_row(2, "Version", PHP_SPIDERMONKEY_EXTVER);
    //php_info_print_table_row(2, "LibJS Version", JS_VersionToString(JS_VERSION));
    php_info_print_table_row(2, "LibJS Version", JS_GetImplementationVersion());
    php_info_print_table_end();
}

zval *jsval_to_zval(zval *return_value, JSContext *ctx, jsval *jval)
{
    jsval   rval;
//    zval    *return_value;

    memcpy(&rval, jval, sizeof(jsval));

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
        str = JS_ValueToString(ctx, rval);
        if (str != NULL)
        {
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
        JSObject    **obj;
        jsid        *idp;
        int         i;

        // create stdClass
        object_init_ex(return_value, ZEND_STANDARD_CLASS_DEF_PTR);

        JS_ValueToObject(ctx, *rval, 

        // then iterate on each property
        it = JS_Enumerate(ctx, rval);

        for (i = 0; i < it->length; i++)
        {
            //php_printf("hello\n");
            jsval val;
            if (JS_IdToValue(ctx, it->vector[i], &val) == JS_TRUE)
            {
                jsdouble d;
                JS_ValueToNumber(ctx, val, &d);
                php_printf("val: %d\n", d);
            }
        }

        JS_DestroyIdArray(ctx, it);
    }
    else if (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval))
    {
        RETVAL_NULL();
    }
    else
        RETVAL_FALSE;

    return return_value;
}
