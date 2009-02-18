#ifndef PHP_SPIDERMONKEY_H
/* Prevent double inclusion */
#define PHP_SPIDERMONKEY_H

/* Define Extension Properties */
#define PHP_SPIDERMONKEY_EXTNAME	"spidermonkey"
#define PHP_SPIDERMONKEY_MINFO_NAME "SpiderMonkey"
#define PHP_SPIDERMONKEY_EXTVER		"0.7"

/* Import configure options
   when building outside of
   the PHP source tree */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Include PHP Standard Header */
#include "php.h"
#define XP_UNIX
/* Include JSAPI Header */
#include "jsapi.h"

#define PHP_SPIDERMONKEY_JSR_NAME			"JSRuntime"
#define PHP_JSRUNTIME_DESCRIPTOR_RES_NAME   "Javascript Runtime"
#define PHP_JSRUNTIME_GC_MEMORY_THRESHOLD   8L * 1024L * 1024L

#define PHP_SPIDERMONKEY_JSC_NAME			"JSContext"
#define PHP_JSCONTEXT_DESCRIPTOR_RES_NAME   "Javascript Context"

/************************
* EXTENSION INTERNALS
************************/

/* Used by JSContext to store callbacks */
typedef struct _php_callback {
	zend_fcall_info			fci;
	zend_fcall_info_cache   fci_cache;
} php_callback;

/* Structure for JSRuntime object. */
typedef struct _php_jsruntime_object  {
	zend_object				zo;
	JSRuntime				*rt;
} php_jsruntime_object;

/* Structure for JSContext object. */
typedef struct _php_jscontext_object  {
	zend_object				zo;
	zval					*rt_z;
	php_jsruntime_object	*rt;
	HashTable				*ht;
	/* Javascript related stuff */
	JSContext				*ct;
	JSClass					script_class;
	JSObject				*obj;
	JSFunctionSpec			global_functions[2];
} php_jscontext_object;

extern zend_class_entry *php_spidermonkey_jsr_entry;
extern zend_class_entry *php_spidermonkey_jsc_entry;

/* this method defined in spidermonkey.c allow us to convert a jsval
 * to a zval for PHP use */
void jsval_to_zval(zval *return_value, JSContext *ctx, jsval *jval);
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval);

/* init/shutdown functions */
PHP_MINIT_FUNCTION(spidermonkey);
PHP_MSHUTDOWN_FUNCTION(spidermonkey);
PHP_MINFO_FUNCTION(spidermonkey);
/* JSRuntime methods */
PHP_METHOD(JSRuntime,   __construct);
PHP_METHOD(JSRuntime,   createContext);
/* JSContext methods */
PHP_METHOD(JSContext,   __construct);
PHP_METHOD(JSContext,   __destruct);
PHP_METHOD(JSContext,	evaluateScript);
PHP_METHOD(JSContext,   registerFunction);
PHP_METHOD(JSContext,   assign);
PHP_METHOD(JSContext,   setOptions);
PHP_METHOD(JSContext,   toggleOptions);
PHP_METHOD(JSContext,   getOptions);
PHP_METHOD(JSContext,   setVersion);
PHP_METHOD(JSContext,   getVersion);
PHP_METHOD(JSContext,   getVersionString);

/**
 * {{{
 * Those methods are directly available to the javascript
 * allowing extended functionnality and communication with
 * PHP. You need to declare them in the global_functions
 * struct in JSContext's constructor
 */
JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/* }}} */

/* Methods used/exported in JS */
void reportError(JSContext *cx, const char *message, JSErrorReport *report);
JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

/* Define the entry point symbol
 * Zend will use when loading this module
 */
extern zend_module_entry spidermonkey_module_entry;
#define phpext_spidermonkey_ptr &spidermonkey_module_entry

#endif /* PHP_SPIDERMONKEY_H */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
