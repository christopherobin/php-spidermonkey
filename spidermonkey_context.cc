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

#include "php_spidermonkey.h"
#include "zend_exceptions.h"

/**
 * JSContext embedding
 */
zend_class_entry *php_spidermonkey_jsc_entry;

// todo: might use zend_closure to fix closure error on test cases.
// https://github.com/php/php-src/blob/1189fe572944932b4d517aa8039596fb1892d5b7/Zend/zend_closures.c
/*
PHP_METHOD(JSContext, registerClosure)
{
}
*/

/* {{{ proto public bool JSContext::registerFunction(string name, callback function)
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerFunction)
{
	zend_string				*name = NULL;
	php_callback			*callback;
	php_jscontext_object	*intern;

	// efree(callback) on JS_FinalizePHP
	callback = (php_callback*)ecalloc(1, sizeof(php_callback));
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "f|S", &callback->fci, &callback->fci_cache, &name) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
	
	PHPJS_START(intern->ct);

	/* TODO: error management is needed here, we should throw an exception if the "name" entry
	 *        already exists */
	if (name == NULL) {
		name = Z_STR(callback->fci.function_name);
	}

	zend_hash_add_new_ptr(intern->jsref->ht, name, callback);
	JS_DefineFunction(intern->ct, intern->obj, ZSTR_VAL(name), generic_call, 1, 0);
	
	PHPJS_END(intern->ct);
}
/* }}} */

/* {{{ proto public bool JSContext::registerClass(string class_name [, string exported_name ])
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerClass)
{
	zend_string				*class_name = NULL;
	zend_string				*exported_name = NULL;
	php_jscontext_object	*intern;
	zend_class_entry		*ce = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S", &class_name, &exported_name) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
	
	PHPJS_START(intern->ct);

	if (ZSTR_LEN(class_name)) {
		ce = zend_lookup_class(class_name);
		if (ce == NULL) {
			PHPJS_END(intern->ct);
			zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0, "Class %s doesn't exists !", ZSTR_VAL(class_name));
			return;
		}
	}

	JSClass *reClass = (JSClass*)emalloc(sizeof(JSClass));
	memcpy(reClass, &intern->script_class, sizeof(intern->script_class));
	if (exported_name != NULL) {
		zend_hash_add_new_ptr(intern->ec_ht, exported_name, ce);
		reClass->name = ZSTR_VAL(exported_name);
	} else {
		zend_hash_add_new_ptr(intern->ec_ht, class_name, ce);
		reClass->name = ZSTR_VAL(class_name);
	}

	JS_InitClass(intern->ct, intern->obj, nullptr, reClass, generic_constructor, 1, nullptr, nullptr, nullptr, nullptr);

	efree(reClass);
	
	/* TODO: error management is needed here, we should throw an exception if the "name" entry
	 *        already exists */
	/*if (exported_name != NULL) {
		zend_hash_add(intern->ec_ht, exported_name, exported_name_len, &ce, sizeof(zend_class_entry**), NULL);
		JS_DefineFunction(intern->ct, intern->obj, exported_name, generic_constructor, 1, 0);
	}
	else {
		zend_hash_add(intern->ec_ht, class_name, class_name_len, &ce, sizeof(zend_class_entry**), NULL);
		//JS_DefineFunction(intern->ct, intern->obj, class_name, generic_constructor, 1, 0);
		JS_InitClass(intern->ct, intern->obj, nullptr, &intern->script_class, generic_constructor, 1, nullptr, nullptr, nullptr, nullptr);
	}*/

	PHPJS_END(intern->ct);
}
/* }}} */


/* {{{ proto public bool JSContext::assign(string name, mixed value)
   Register a value in Javascript's global scope*/
PHP_METHOD(JSContext, assign)
{
	zend_string				*name;
	zval					*val;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sz", &name, &val) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
	php_jsobject_set_property(intern->ct, intern->obj, ZSTR_VAL(name), val);
}
/* }}} */

/* {{{ proto public mixed JSContext::evaluateScript(string $script [, string $script_name ])
   Evaluate script and return the last global object in scope to PHP.
   Objects are not returned for now. Any global variable declared in
   this script will be usable in any following call to evaluateScript
   in any JSObject in  the same context. */
PHP_METHOD(JSContext, evaluateScript)
{
	zend_string *script;
	
	php_jscontext_object *intern;
	jsval rval;

	/* retrieve script */
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &script) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
	
	PHPJS_START(intern->ct);

	if (JS_EvaluateScript(intern->ct, intern->obj, ZSTR_VAL(script), ZSTR_LEN(script), "evaluateScript", 0, &rval) == JS_TRUE)
	{
		if (!rval.isNullOrUndefined())
		{
			/* The script evaluated fine, convert the return value to PHP */
			jsval_to_zval(return_value, intern->ct, JS::MutableHandleValue::fromMarkedLocation(&rval));
		}
		else
			RETVAL_NULL();
		/* run garbage collection */
		JS_MaybeGC(intern->ct);
	}
	else
	{
		RETVAL_FALSE;
	}
	
	PHPJS_END(intern->ct);
}
/* }}} */

/* {{{ proto public mixed JSContext::setOptions(long $options)
   Set options for the current context, $options is a bitfield made of JSOPTION_ consts */
PHP_METHOD(JSContext, setOptions)
{
	php_jscontext_object	*intern;
	long					options;
	long					old_options;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &options) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
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
/* }}} */

/* {{{ proto public mixed JSContext::toggleOptions(long $options)
   Just toggle a set of options, same as doing setOptions 
   with JSContext::getOptions() ^ $options */
PHP_METHOD(JSContext, toggleOptions)
{
	php_jscontext_object	*intern;
	long					options;
	long					old_options;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &options) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
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
/* }}} */

/* {{{ proto public long JSContext::getOptions()
   Return context's options */
PHP_METHOD(JSContext, getOptions)
{
	php_jscontext_object	*intern;

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());

	RETVAL_LONG(JS_GetOptions(intern->ct));
}
/* }}} */

/* {{{ proto public mixed JSContext::setVersion(long $version)
   Set Javascript version for this context, supported versions are
   dependant of the current spidermonkey library */
PHP_METHOD(JSContext, setVersion)
{
	php_jscontext_object	*intern;
	long					version;
	long					old_version;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &version) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());
//	old_version = JS_SetVersion(intern->ct, version);
	
	if (JS_GetVersion(intern->ct) == version)
	{
		RETVAL_LONG(old_version);
	}
	else
	{
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto public long JSContext::getVersion(long $options)
   Return current version */
PHP_METHOD(JSContext, getVersion)
{
	php_jscontext_object	*intern;

	intern = (php_jscontext_object *) Z_OBJ_P(getThis());

	RETVAL_LONG(JS_GetVersion(intern->ct));
}
/* }}} */

/* {{{ proto public string JSContext::getVersionString(long $version)
   Return the version name base on his number*/
PHP_METHOD(JSContext, getVersionString)
{
	const char *version_str;
	zend_long version;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &version) == FAILURE) {
		RETURN_NULL();
	}

	version_str = JS_VersionToString((JSVersion)version);
	RETVAL_STRINGL(version_str, strlen(version_str));
}
/* }}} */

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
