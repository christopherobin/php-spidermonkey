#include "php_spidermonkey.h"

static int le_jscontext_descriptor;

/**
 * JSContext embedding
 */
zend_class_entry *php_spidermonkey_jsc_entry;

/* {{{ proto public JSContext::__construct(JSRuntime $runtime)
   JSContext's constructor, expect a JSRuntime, you should use
   JSRuntime::createContext */
PHP_METHOD(JSContext, __construct)
{
	/* prevent creating this object */
	//zend_throw_exception(zend_exception_get_default(TSRMLS_C), "JSContext can't be instancied directly, please call JSRuntime::createContext", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto public bool JSContext::registerFunction(string name, callback function)
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerFunction)
{
	char					*name;
	int						name_len;
	php_callback			callback;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &callback.fci, &callback.fci_cache) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	Z_ADDREF_P(callback.fci.function_name);

	/* TODO: error management is needed here, we should throw an exception if the "name" entry
	 *        already exists */
	zend_hash_add(intern->jsref->ht, name, name_len, &callback, sizeof(callback), NULL);

	JS_DefineFunction(intern->ct, intern->obj, name, generic_call, 1, 0);

}
/* }}} */

/* {{{ proto public bool JSContext::registerFunction(string name, callback function)
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, assign)
{
	char					*name;
	int						name_len;
	zval					*val;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &name_len, &val) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	jsval ival;
	zval_to_jsval(val, intern->ct, &ival);

	JS_SetProperty(intern->ct, intern->obj, name, &ival);
}
/* }}} */

/* {{{ proto public mixed JSContext::evaluateScript(string $script)
   Evaluate script and return the last global object in scope to PHP.
   Objects are not returned for now. Any global variable declared in
   this script will be usable in any following call to evaluateScript
   in any JSObject in  the same context. */
PHP_METHOD(JSContext, evaluateScript)
{
	char *script;
	int script_len;
	php_jscontext_object *intern;
	jsval rval;

	/* retrieve script */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						"s", &script, &script_len) == FAILURE) {
		RETURN_FALSE;
	}

	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (JS_EvaluateScript(intern->ct, intern->obj, script, script_len, NULL, 0, &rval) == JS_TRUE)
	{
		/* The script evaluated fine, convert the return value to PHP */
		jsval_to_zval(return_value, intern->ct, &rval);
	}
	else
	{
		RETURN_FALSE;
	}

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
	old_version = JS_SetVersion(intern->ct, version);
	
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

	version_str = JS_VersionToString(version);
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
