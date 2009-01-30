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

    JS_InitStandardClasses(intern_ct->ct, intern_ot->obj);

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

PHP_METHOD(JSObject, evaluateScript)
{
    char *script;
    int script_len;
    php_jsobject_object *intern;
    jsval rval;
    JSBool ok;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                         "s", &script, &script_len) == FAILURE) {
        RETURN_FALSE;
    }

    intern = (php_jsobject_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

    if (JS_EvaluateScript(intern->ct->ct, intern->obj, script, script_len, "spidermonkey_object.c", 60, &rval) == JS_TRUE)
    {
        if (JSVAL_IS_NUMBER(rval) || JSVAL_IS_DOUBLE(rval))
        {
            jsdouble d;
            if (JS_ValueToNumber(intern->ct->ct, rval, &d) == JS_TRUE)
            {
                RETVAL_DOUBLE(d);
            }
            else
                RETURN_FALSE;
        }
        else if (JSVAL_IS_STRING(rval))
        {
            JSString *str;
            str = JS_ValueToString(intern->ct->ct, rval);
//            php_printf("str: %p (%d)\n", str, JSSTRING_LENGTH(str));
            if (str != NULL)
            {
                char *txt = JS_GetStringBytes(str);
                RETVAL_STRING(txt, strlen(txt));
            }
            else
            {
                RETURN_FALSE;
            }
        }
        else
            RETURN_FALSE;
    }
    else
    {
        RETURN_FALSE;
    }

}

