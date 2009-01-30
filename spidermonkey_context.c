#include "php_spidermonkey.h"

static int le_jscontext_descriptor;

/**
* JSContext embedding
*/

zend_class_entry *php_spidermonkey_jsc_entry;

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

    return;
}

PHP_METHOD(JSContext, __destruct)
{
    php_jscontext_object *intern_ct;

    intern_ct = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (intern_ct->ct != (JSContext*)NULL)
        JS_DestroyContext(intern_ct->ct);

    intern_ct->rt = NULL;
}
