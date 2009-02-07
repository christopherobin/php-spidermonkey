#include "php_spidermonkey.h"

static int le_jscontext_descriptor;

/**
 * {{{
 * Those methods are directly available to the javascript
 * allowing extended functionnality and communication with
 * PHP. You need to declare them in the global_functions
 * struct in JSContext's constructor
 */
JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
/* }}} */

/**
 * JSContext embedding
 */
zend_class_entry *php_spidermonkey_jsc_entry;

/* The error reporter callback. */
/* TODO: change that to an exception */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
	php_printf("%s:%u:%s\n",
			report->filename ? report->filename : "<no filename>",
			(unsigned int) report->lineno,
			message);
}

/* {{{ proto public JSContext::__construct(JSRuntime $runtime)
   JSContext's constructor, expect a JSRuntime, you should use
   JSRuntime::createContext */
PHP_METHOD(JSContext, __construct)
{
	php_jsruntime_object *intern_rt;
	php_jscontext_object *intern_ct;
	zval *z_rt;

	/* parse parameters, this function is designed to receive a JSRuntime only */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
						 "O", &z_rt, php_spidermonkey_jsr_entry) == FAILURE) {
   		zend_error(E_ERROR, "Missing argument JSRuntime", php_spidermonkey_jsc_entry->name);
		RETURN_NULL();
	}

	/* retrieve objects from object store */
	intern_rt = (php_jsruntime_object *) zend_object_store_get_object(z_rt TSRMLS_CC);
	intern_ct = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	intern_ct->rt = intern_rt;
	/* create a new context */
	intern_ct->ct = JS_NewContext(intern_rt->rt, 8092);

	/* The script_class is a global object used by PHP to offer methods */
	intern_ct->script_class.name			= "script";
	intern_ct->script_class.flags		   = JSCLASS_GLOBAL_FLAGS;

	/* Mandatory non-null function pointer members. */
	intern_ct->script_class.addProperty	 = JS_PropertyStub;
	intern_ct->script_class.delProperty	 = JS_PropertyStub;
	intern_ct->script_class.getProperty	 = JS_PropertyStub;
	intern_ct->script_class.setProperty	 = JS_PropertyStub;
	intern_ct->script_class.enumerate	   = JS_EnumerateStub;
	intern_ct->script_class.resolve		 = JS_ResolveStub;
	intern_ct->script_class.convert		 = JS_ConvertStub;
	intern_ct->script_class.finalize		= JS_FinalizeStub;

	/* Optionally non-null members start here. */
	intern_ct->script_class.getObjectOps	= 0;
	intern_ct->script_class.checkAccess	 = 0;
	intern_ct->script_class.call			= 0;
	intern_ct->script_class.construct	   = 0;
	intern_ct->script_class.xdrObject	   = 0;
	intern_ct->script_class.hasInstance	 = 0;
	intern_ct->script_class.mark			= 0;
	intern_ct->script_class.reserveSlots	= 0;

	/* register global functions */
	intern_ct->global_functions[0].name	 = "write";
	intern_ct->global_functions[0].call	 = script_write;
	intern_ct->global_functions[0].nargs	= 1;
	intern_ct->global_functions[0].flags	= 0;
	intern_ct->global_functions[0].extra	= 0;

	/* last element of the list need to be zeroed */
	memset(&intern_ct->global_functions[1], 0, sizeof(JSFunctionSpec));

	/* says that our script runs in global scope */
	JS_SetOptions(intern_ct->ct, JSOPTION_VAROBJFIX);

	/* set the error callback */
	JS_SetErrorReporter(intern_ct->ct, reportError);

	return;
}
/* }}} */

/* {{{ proto public void JSContext::__destruct()
   Destroy context and free up memory */
PHP_METHOD(JSContext, __destruct)
{
	php_jscontext_object *intern;

	/* retrieve this class from the store */
	intern = (php_jscontext_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	/* if a context is found ( which should be the case )
	 * destroy it
	 */
	if (intern->ct != (JSContext*)NULL)
		JS_DestroyContext(intern->ct);

	intern->rt = NULL;
}
/* }}} */

/* {{{ proto public JSObject JSContext::createObject()
   Returns an instance of JSObject using this context */
PHP_METHOD(JSContext, createObject)
{
	zval *retval_ptr = NULL;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *this;
	zval **params[1];

	/* "this" is gonna contain a reference to our class, we need
	 * to build a zval, and set the handle to our object */
	MAKE_STD_ZVAL(this);
	this->type = IS_OBJECT;
	this->is_ref = 1;
	this->value.obj.handle = (getThis())->value.obj.handle;
	this->value.obj.handlers = (getThis())->value.obj.handlers;
	zval_copy_ctor(this);

	/* Then we set the first param to be this zval */
	params[0] = &this;

	/* init object */
	object_init_ex(return_value, php_spidermonkey_jso_entry);

	if (php_spidermonkey_jso_entry->constructor)
	{
		/* first the function call info struct */
		fci.size = sizeof(fci);
		fci.function_table = EG(function_table);
		fci.function_name = NULL;
		fci.symbol_table = NULL;
		fci.object_pp = &return_value;
		fci.retval_ptr_ptr = &retval_ptr;
		fci.param_count = 1;
		fci.params = params;
		fci.no_separation = 1;

		/* then the cache */
		fcc.initialized = 1;
		fcc.function_handler = php_spidermonkey_jso_entry->constructor;
		fcc.calling_scope = EG(scope);
		fcc.object_pp = &return_value;

		/* call JSObject's constructor */
		if (zend_call_function(&fci, &fcc TSRMLS_CC) == FAILURE) {
			if (retval_ptr) {
				zval_ptr_dtor(&retval_ptr);
			}
			zval_ptr_dtor(&this);
			/* this should *never* happen ! */
			zend_error(E_ERROR, "Invocation of JSObject's constructor failed", php_spidermonkey_jso_entry->name);
			RETURN_NULL();
		}

		if (retval_ptr) {
			zval_ptr_dtor(&retval_ptr);
		}
	}
	
	zval_ptr_dtor(&this);
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

JSBool script_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *str;
	str = JS_ValueToString(cx, argv[0]);
	char *txt = JS_GetStringBytes(str);
	php_printf("%s", txt);

	return JSVAL_TRUE;
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
