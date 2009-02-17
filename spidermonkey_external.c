#include "php_spidermonkey.h"

/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	/* throw error */
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), message, 0 TSRMLS_CC);
}


JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *str;
	str = JS_ValueToString(cx, argv[0]);
	char *txt = JS_GetStringBytes(str);
	php_printf("%s", txt);

	return JSVAL_TRUE;
}

