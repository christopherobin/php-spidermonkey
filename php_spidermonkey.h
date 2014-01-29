/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Christophe Robin <crobin@php.net>                            |
  +----------------------------------------------------------------------+

  $Id$
  $Revision$
*/

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
extern "C" {
#include "php.h"
}

/* Include JSAPI Header */
#define XP_UNIX
#include "jsapi.h"

#define PHP_JSRUNTIME_GC_MEMORY_THRESHOLD   "8388608"

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

/* those are necessary for threadsafe builds since 1.8.5 */
#define PHPJS_START(cx) JS_BeginRequest(cx)
#define PHPJS_END(cx) JS_EndRequest(cx)

// useful for iterating on php hashtables
#define PHPJS_FOREACH(ht) for (zend_hash_internal_pointer_reset(ht); zend_hash_has_more_elements(ht) == SUCCESS; zend_hash_move_forward(ht))
#define PHPJS_FOREACH_ENTRY(ht,dest) if (zend_hash_get_current_data(ht,(void**)& dest) == FAILURE) {\
  continue;\
}

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
	JSClass					global_class;
	JSClass					script_class;
	JSObject				*obj;
	JSCompartment			*cpt;
} php_jscontext_object;

typedef struct _php_jsparent {
    JSObject                *obj;
    zval                    *zobj;
    struct _php_jsparent    *parent;
} php_jsparent;

extern zend_class_entry *php_spidermonkey_jsc_entry;

/* this method defined in spidermonkey.c allow us to convert a jsval
 * to a zval for PHP use */
void php_jsobject_set_property(JSContext *ctx, JSObject *obj, char *property_name, zval *val TSRMLS_DC);
#define jsval_to_zval(rval, ctx, jval) _jsval_to_zval(rval, ctx, jval, NULL TSRMLS_CC)
void _jsval_to_zval(zval *return_value, JSContext *ctx, JS::MutableHandle<JS::Value> rval, php_jsparent *parent TSRMLS_DC);
void zval_to_jsval(zval *val, JSContext *ctx, jsval *jval TSRMLS_DC);

/* init/shutdown functions */
PHP_MINIT_FUNCTION(spidermonkey);
PHP_MSHUTDOWN_FUNCTION(spidermonkey);
PHP_RSHUTDOWN_FUNCTION(spidermonkey);
PHP_MINFO_FUNCTION(spidermonkey);
/* For intercepting late ini modification */
PHP_INI_MH(spidermonkey_ini_update);
/* JSContext methods */
PHP_METHOD(JSContext,	evaluateScript);
PHP_METHOD(JSContext, registerFunction);
PHP_METHOD(JSContext, registerClass);
PHP_METHOD(JSContext, assign);
PHP_METHOD(JSContext, setOptions);
PHP_METHOD(JSContext, toggleOptions);
PHP_METHOD(JSContext, getOptions);
PHP_METHOD(JSContext, setVersion);
PHP_METHOD(JSContext, getVersion);
PHP_METHOD(JSContext, getVersionString);

/**
 * {{{
 * Those methods are directly available to the javascript
 * allowing extended functionnality and communication with
 * PHP. You need to declare them in the global_functions
 * struct in JSContext's constructor
 */
JSBool generic_call(JSContext *cx, unsigned argc, JS::Value *vp);
JSBool generic_constructor(JSContext *cx, unsigned argc, JS::Value *vp);
/* }}} */

/* Methods used/exported in JS */
void reportError(JSContext *cx, const char *message, JSErrorReport *report);
void JS_FinalizePHP(JSFreeOp *fop, JSObject *obj);
JSBool JS_ResolvePHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id);
JSBool JS_PropertyGetterPHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
        JS::MutableHandle<JS::Value> vp);
JSBool JS_PropertySetterPHP(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JSBool strict,
        JS::MutableHandle<JS::Value> vp);

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
