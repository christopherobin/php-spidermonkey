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

/* {{{ proto public bool JSContext::registerFunction(string name, callback function)
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerFunction)
{
	char					*name = NULL;
	int						name_len = 0;
	php_callback			callback;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f|s", &callback.fci, &callback.fci_cache, &name, &name_len) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	
	PHPJS_START(intern->ct);

	Z_ADDREF_P(callback.fci.function_name);

	/* TODO: error management is needed here, we should throw an exception if the "name" entry
	 *        already exists */
	if (name == NULL) {
		name		= Z_STRVAL_P(callback.fci.function_name);
		name_len	= Z_STRLEN_P(callback.fci.function_name);
	}

	zend_hash_add(intern->jsref->ht, name, name_len, &callback, sizeof(callback), NULL);
	JS_DefineFunction(intern->ct, intern->obj, name, generic_call, 1, 0);
	
	PHPJS_END(intern->ct);
}
/* }}} */

/* {{{ proto public bool JSContext::registerClass(string class_name [, string exported_name ])
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerClass)
{
	char					*class_name = NULL;
	int						class_name_len = 0;
	char					*exported_name = NULL;
	int						exported_name_len = 0;
	php_jscontext_object	*intern;
	zend_class_entry		*ce = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &class_name, &class_name_len, &exported_name, &exported_name_len) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	
	PHPJS_START(intern->ct);

	if (class_name_len) {
		zend_class_entry **pce;
		if (zend_lookup_class(class_name, class_name_len, &pce TSRMLS_CC) == FAILURE) {
			if (!EG(exception)) {
				PHPJS_END(intern->ct);
				zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 0 TSRMLS_CC, "Class %s doesn't exists !", class_name);
				return;
			}
		}
		ce = *pce;
	}

	/* TODO: error management is needed here, we should throw an exception if the "name" entry
	 *        already exists */
	if (exported_name != NULL) {
		zend_hash_add(intern->ec_ht, exported_name, exported_name_len, &ce, sizeof(zend_class_entry**), NULL);
		JS_DefineFunction(intern->ct, intern->obj, exported_name, generic_constructor, 1, 0);
	}
	else {
		zend_hash_add(intern->ec_ht, class_name, class_name_len, &ce, sizeof(zend_class_entry**), NULL);
		JS_DefineFunction(intern->ct, intern->obj, class_name, generic_constructor, 1, 0);
	}

	PHPJS_END(intern->ct);
}
/* }}} */


/* {{{ proto public bool JSContext::assign(string name, mixed value)
   Register a value in Javascript's global scope*/
PHP_METHOD(JSContext, assign)
{
	char					*name;
	int						name_len;
	zval					*val = NULL;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &name_len, &val) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	php_jsobject_set_property(intern->ct, intern->obj, name, val TSRMLS_CC);
}
/* }}} */

/* {{{ proto public mixed JSContext::evaluateScript(string $script [, string $script_name ])
   Evaluate script and return the last global object in scope to PHP.
   Objects are not returned for now. Any global variable declared in
   this script will be usable in any following call to evaluateScript
   in any JSObject in  the same context. */
PHP_METHOD(JSContext, evaluateScript)
{
	char *script;
	char *script_name = NULL;
	int script_len, script_name_len = 0;
	php_jscontext_object *intern;
	jsval rval;

	/* retrieve script */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						"s|s", &script, &script_len, &script_name, &script_name_len) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	
	PHPJS_START(intern->ct);
	
	if (JS_EvaluateScript(intern->ct, intern->obj, script, script_len, script_name, 0, &rval) == JS_TRUE)
	{
		if (rval != 0)
		{
			/* The script evaluated fine, convert the return value to PHP */
			jsval_to_zval(return_value, intern->ct, &rval);
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

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						 "l", &options) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
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

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						 "l", &options) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
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

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

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

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						 "l", &version) == FAILURE) {
		RETURN_NULL();
	}

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	old_version = JS_SetVersion(intern->ct, (JSVersion)version);
	
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

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	RETVAL_LONG(JS_GetVersion(intern->ct));
}
/* }}} */

/* {{{ proto public string JSContext::getVersionString(long $version)
   Return the version name base on his number*/
PHP_METHOD(JSContext, getVersionString)
{
	const char *version_str;
	int l;
	long version;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						 "l", &version) == FAILURE) {
		RETURN_NULL();
	}

	version_str = JS_VersionToString((JSVersion)version);
	l = strlen(version_str);

	RETVAL_STRINGL(estrndup(version_str, l), l, 0);
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
