#include "php_spidermonkey.h"

static int le_jsobject_descriptor;

/**
* JSContext embedding
*/

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

    intern_ot->ct = intern_ct;
    intern_ot->obj = JS_NewObject(intern_ct->ct, NULL, NULL, NULL);

    return;
}

PHP_METHOD(JSObject, __destruct)
{
    php_jsobject_object *intern_ot;

    intern_ot = (php_jsobject_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    /*if (intern_ct->obj != (JSObject*)NULL)
        JS_DestroyContext(intern_ct->ct);*/

    intern_ot->obj = NULL; // the context destroy the objects itself
    intern_ot->ct = NULL;
}
