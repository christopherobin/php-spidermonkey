#include "php_spidermonkey.h"

static int le_jsobject_descriptor;

/**
* JSObject embedding
*/

/* The class of the global object. */
//static 

zend_class_entry *php_spidermonkey_jso_entry;

PHP_METHOD(JSObject, __construct)
{
    php_jscontext_object    *intern_ct;
    php_jsobject_object     *intern_ot;
    zval *z_rt;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "O", &z_rt, php_spidermonkey_jsc_entry) == FAILURE) {
        RETURN_NULL();
    }

    intern_ct = (php_jscontext_object *) zend_object_store_get_object(z_rt TSRMLS_CC);
    intern_ot = (php_jsobject_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    /* Other thingy */
    intern_ot->ct = intern_ct;
    intern_ot->obj = JS_NewObject(intern_ct->ct, &intern_ct->script_class, NULL, NULL);

    // register globals functions
    JS_DefineFunctions(intern_ct->ct, intern_ot->obj, intern_ct->global_functions);

    JS_InitStandardClasses(intern_ct->ct, intern_ot->obj);

    return;
}

PHP_METHOD(JSObject, __destruct)
{
    php_jsobject_object *intern_ot;

    intern_ot = (php_jsobject_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    intern_ot->obj = NULL; // the context destroy the objects itself
    intern_ot->ct = NULL;
}

PHP_METHOD(JSObject, evaluateScript)
{
    char *script;
    int script_len;
    php_jsobject_object *intern;
    jsval rval;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "s", &script, &script_len) == FAILURE) {
        RETURN_FALSE;
    }

    intern = (php_jsobject_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (JS_EvaluateScript(intern->ct->ct, intern->obj, script, script_len, "spidermonkey_object.c", 60, &rval) == JS_TRUE)
    {
        jsval_to_zval(return_value, intern->ct->ct, &rval);
    }
    else
    {
        RETURN_FALSE;
    }

}

