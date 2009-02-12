#include "php_spidermonkey.h"

static int le_jscontext_descriptor;

/**
 * {{{
 * Those methods are directly available to the javascript
 * allowing extended functionnality and communication with
 * PHP. You need to declare them in the global_functions
 * struct in JSContext's constructor
 */
//JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/* }}} */

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
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), "JSContext can't be instancied directly, please call JSRuntime::createContext", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto public void JSContext::__destruct()
   Destroy context and free up memory */
PHP_METHOD(JSContext, __destruct)
{
	php_jscontext_object *intern;

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	ZVAL_DELREF(intern->rt_z);

	/* if a context is found ( which should be the case )
	 * destroy it
	 */
	if (intern->ct != (JSContext*)NULL)
		JS_DestroyContext(intern->ct);

	intern->rt = NULL;
}
/* }}} */

/* {{{ proto public bool JSContext::registerFunction(string name, callback function)
   Register a PHP function in a Javascript context allowing a script to call it*/
PHP_METHOD(JSContext, registerFunction)
{
	char					*name;
	int						name_len;
	zend_fcall_info			fci;
	zend_fcall_info_cache	fci_cache;
	php_jscontext_object	*intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sf", &name, &name_len, &fci, &fci_cache) == FAILURE) {
		RETURN_NULL();
	}

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	intern->n_exported_functions++;

	if (intern->fcis)
	{
		intern->fcis		= (zend_fcall_info*)erealloc(intern->fcis, intern->n_exported_functions*sizeof(zend_fcall_info));
		intern->fcis_cache  = (zend_fcall_info_cache*)erealloc(intern->fcis_cache, intern->n_exported_functions*sizeof(zend_fcall_info_cache));
	}
	else
	{
		intern->fcis		= (zend_fcall_info*)emalloc(intern->n_exported_functions*sizeof(zend_fcall_info));
		intern->fcis_cache  = (zend_fcall_info_cache*)emalloc(intern->n_exported_functions*sizeof(zend_fcall_info_cache));
	}

	memcpy(&intern->fcis[intern->n_exported_functions-1], &fci, sizeof(fci));
	memcpy(&intern->fcis_cache[intern->n_exported_functions-1], &fci_cache, sizeof(fci_cache));
}
/* }}} */

/* {{{ proto public JSObject JSContext::createObject()
   Returns an instance of JSObject using this context */
PHP_METHOD(JSContext, createObject)
{
	php_jscontext_object	*intern_ct;
	php_jsobject_object		*intern_ot;

	/* first create JSObject item */
	object_init_ex(return_value, php_spidermonkey_jso_entry);

	/* retrieve objects from store */
	intern_ct = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	intern_ot = (php_jsobject_object *) zend_object_store_get_object(return_value TSRMLS_CC);

	/* store this and increment refcount */
	intern_ot->ct_z = getThis();
	ZVAL_ADDREF(intern_ot->ct_z);

	/* store store object for fast retrieval */
	intern_ot->ct = intern_ct;
	intern_ot->obj = JS_NewObject(intern_ct->ct, &intern_ct->script_class, NULL, NULL);

	/* register globals functions */
	JS_DefineFunctions(intern_ct->ct, intern_ot->obj, intern_ct->global_functions);

	/* initialize standard JS classes */
	JS_InitStandardClasses(intern_ct->ct, intern_ot->obj);
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

/*******************************************
* Internal function for the script JS class
*******************************************/


/*JSBool generic_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *str;
	str = JS_ValueToString(cx, argv[0]);
	char *txt = JS_GetStringBytes(str);
	php_printf("%s", txt);

	return JSVAL_TRUE;
}*/

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
