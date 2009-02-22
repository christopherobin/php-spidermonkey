#ifndef PHP_SPIDERMONKEY_H
/* Prevent double inclusion */
#define PHP_SPIDERMONKEY_H

/* Define Extension Properties */
#define PHP_SPIDERMONKEY_EXTNAME	"spidermonkey"
#define PHP_SPIDERMONKEY_MINFO_NAME "SpiderMonkey"
#define PHP_SPIDERMONKEY_EXTVER		"@PACKAGE_VERSION@"

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

#define PHP_JSRUNTIME_GC_MEMORY_THRESHOLD   8L * 1024L * 1024L

#define PHP_SPIDERMONKEY_JSC_NAME			"JSContext"
#define PHP_JSCONTEXT_DESCRIPTOR_RES_NAME   "Javascript Context"

/************************
* EXTENSION INTERNALS
************************/

ZEND_BEGIN_MODULE_GLOBALS(spidermonkey)
	JSRuntime *rt;
ZEND_END_MODULE_GLOBALS(spidermonkey)

#ifdef ZTS
	#define SPIDERMONKEY_G(v) TSRMG(spidermonkey_globals_id, zend_spidermonkey_globals*, v)
#else
	#define SPIDERMONKEY_G(v) (spidermonkey_globals.v)
#endif

/* Used by JSContext to store callbacks */
typedef struct _php_callback {
	zend_fcall_info			fci;
	zend_fcall_info_cache   fci_cache;
} php_callback;

/* Used by JSObject to refer to their parent object */
typedef struct _php_jsobject_ref {
	/* contain the function list for this object */
	HashTable				*ht;
	/* contain the original resource/object this JSObject was created from */
	zval					*obj;
} php_jsobject_ref;

/* Structure for JSContext object. */
typedef struct _php_jscontext_object  {
	zend_object				zo;
	zval					*rt_z;
	php_jsobject_ref		*jsref;
	/* exported class list */
	HashTable				*ec_ht;
	/* Javascript related stuff */
	JSContext				*ct;
	JSClass					script_class;
	JSObject				*obj;
} php_jscontext_object;

extern zend_class_entry *php_spidermonkey_jsc_entry;

/* this method defined in spidermonkey.c allow us to convert a jsval
 * to a zval for PHP use */
void php_jsobject_set_property(JSContext *ctx, JSObject *obj, char *property_name, zval *val);
void jsval_to_zval(zval *return_value, JSContext *ctx, jsval *jval);
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval);

/* init/shutdown functions */
PHP_MINIT_FUNCTION(spidermonkey);
PHP_MSHUTDOWN_FUNCTION(spidermonkey);
PHP_MINFO_FUNCTION(spidermonkey);
/* JSContext methods */
PHP_METHOD(JSContext,   __construct);
PHP_METHOD(JSContext,	evaluateScript);
PHP_METHOD(JSContext,   registerFunction);
PHP_METHOD(JSContext,   registerClass);
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
JSBool generic_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/* }}} */

/* Methods used/exported in JS */
void reportError(JSContext *cx, const char *message, JSErrorReport *report);
void JS_FinalizePHP(JSContext *cx, JSObject *obj);
JSBool JS_ResolvePHP(JSContext *cx, JSObject *obj, jsval id);
JSBool JS_PropertySetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
JSBool JS_PropertyGetterPHP(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

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
