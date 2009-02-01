#include "php_spidermonkey.h"

static int le_jscontext_descriptor;

JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/**
* JSContext embedding
*/
zend_class_entry *php_spidermonkey_jsc_entry;

/* The error reporter callback. */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
    php_printf("%s:%u:%s\n",
            report->filename ? report->filename : "<no filename>",
            (unsigned int) report->lineno,
            message);
}

PHP_METHOD(JSContext, __construct)
{
    php_jsruntime_object *intern_rt;
    php_jscontext_object *intern_ct;
    zval *z_rt;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "O", &z_rt, php_spidermonkey_jsr_entry) == FAILURE) {
        RETURN_NULL();
    }

    intern_rt = (php_jsruntime_object *) zend_object_store_get_object(z_rt TSRMLS_CC);
    intern_ct = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    intern_ct->rt = intern_rt;
    intern_ct->ct = JS_NewContext(intern_rt->rt, 8092);


    // THE SCRIPT CLASS IS A GLOBAL OBJECT USED BY PHP TO OFFER METHODS
    intern_ct->script_class.name            = "script";
    intern_ct->script_class.flags           = JSCLASS_GLOBAL_FLAGS;

    /* Mandatory non-null function pointer members. */
    intern_ct->script_class.addProperty     = JS_PropertyStub;
    intern_ct->script_class.delProperty     = JS_PropertyStub;
    intern_ct->script_class.getProperty     = JS_PropertyStub;
    intern_ct->script_class.setProperty     = JS_PropertyStub;
    intern_ct->script_class.enumerate       = JS_EnumerateStub;
    intern_ct->script_class.resolve         = JS_ResolveStub;
    intern_ct->script_class.convert         = JS_ConvertStub;
    intern_ct->script_class.finalize        = JS_FinalizeStub;

    /* Optionally non-null members start here. */
    intern_ct->script_class.getObjectOps    = 0;
    intern_ct->script_class.checkAccess     = 0;
    intern_ct->script_class.call            = 0;
    intern_ct->script_class.construct       = 0;
    intern_ct->script_class.xdrObject       = 0;
    intern_ct->script_class.hasInstance     = 0;
    intern_ct->script_class.mark            = 0;
    intern_ct->script_class.reserveSlots    = 0;

    // register global functions
    intern_ct->global_functions[0].name     = "write";
    intern_ct->global_functions[0].call     = script_write;
    intern_ct->global_functions[0].nargs    = 1;
    intern_ct->global_functions[0].flags    = 0;
    intern_ct->global_functions[0].extra    = 0;

    // last element
    memset(&intern_ct->global_functions[1], 0, sizeof(JSFunctionSpec));

    // says that our script runs in global scope
    JS_SetOptions(intern_ct->ct, JSOPTION_VAROBJFIX);

    JS_SetErrorReporter(intern_ct->ct, reportError);

    return;
}

PHP_METHOD(JSContext, __destruct)
{
    php_jscontext_object *intern;

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (intern->ct != (JSContext*)NULL)
        JS_DestroyContext(intern->ct);

    intern->rt = NULL;
}

PHP_METHOD(JSContext, setOptions)
{
    php_jscontext_object    *intern;
    long                    options;
    long                    old_options;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "l", &options) == FAILURE) {
        RETURN_NULL();
    }

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    old_options = JS_SetOptions(intern->ct, options);
    
    if (JS_GetOptions(intern->ct) == options)
    {
        RETVAL_LONG(old_options);
    }
    else
    {
        RETURN_FALSE;
    }
}

PHP_METHOD(JSContext, toggleOptions)
{
    php_jscontext_object    *intern;
    long                    options;
    long                    old_options;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "l", &options) == FAILURE) {
        RETURN_NULL();
    }

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    old_options = JS_ToggleOptions(intern->ct, options);
    
    if (JS_GetOptions(intern->ct) == (old_options ^ options))
    {
        RETVAL_LONG(old_options);
    }
    else
    {
        RETURN_FALSE;
    }
}

PHP_METHOD(JSContext, getOptions)
{
    php_jscontext_object    *intern;

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    RETVAL_LONG(JS_GetOptions(intern->ct));
}

PHP_METHOD(JSContext, setVersion)
{
    php_jscontext_object    *intern;
    long                    version;
    long                    old_version;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "l", &version) == FAILURE) {
        RETURN_NULL();
    }

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
    old_version = JS_SetVersion(intern->ct, version);
    
    if (JS_GetVersion(intern->ct) == version)
    {
        RETVAL_LONG(old_version);
    }
    else
    {
        RETURN_FALSE;
    }
}

PHP_METHOD(JSContext, getVersion)
{
    php_jscontext_object    *intern;

    intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    RETVAL_LONG(JS_GetVersion(intern->ct));
}

PHP_METHOD(JSContext, getVersionString)
{
    char *version_str;
    long version;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "l", &version) == FAILURE) {
        RETURN_NULL();
    }

    version_str = JS_VersionToString(version);

    RETVAL_STRING(version_str, strlen(version_str));
}

/*******************************************
* Internal function for the script JS class
*******************************************/

JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    str = JS_ValueToString(cx, argv[0]);
    char *txt = JS_GetStringBytes(str);
    php_printf("%s", txt);

    return JSVAL_TRUE;
}



