#include "php_spidermonkey.h"

/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	php_printf("%s:%u:%s\n",
			report->filename ? report->filename : "<no filename>",
			(unsigned int) report->lineno,
			message);
}


JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *str;
	str = JS_ValueToString(cx, argv[0]);
	char *txt = JS_GetStringBytes(str);
	php_printf("%s", txt);

	return JSVAL_TRUE;
}

