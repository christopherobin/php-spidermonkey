#include "php_spidermonkey.h"

static int le_jsruntime_descriptor;

/**
* JSRuntime embedding
*/
zend_class_entry *php_spidermonkey_jsr_entry;

PHP_METHOD(JSRuntime, __construct)
{
    /*  maybe we should add an array containing options ? */
/*     return SUCCESS; */
}

/* {{{ proto public JSContext JSRuntime::createContext()
   Returns an instance of JSContext using this runtime */
PHP_METHOD(JSRuntime, createContext)
{
	php_jsruntime_object *intern_rt;
	php_jscontext_object *intern;

	/* init JSContext object */
	object_init_ex(return_value, php_spidermonkey_jsc_entry);

	/* retrieve objects from object store */
	intern_rt = (php_jsruntime_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	intern = (php_jscontext_object *) zend_object_store_get_object(return_value TSRMLS_CC);

	/* store this and increment ref count */
	intern->rt_z = getThis();
	ZVAL_ADDREF(intern->rt_z);

	intern->rt = intern_rt;
	/* create a new context */
	intern->ct = JS_NewContext(intern_rt->rt, 8092);
	/* store pointer to intern */
	JS_SetContextPrivate(intern->ct, intern);

	/* The script_class is a global object used by PHP to offer methods */
	intern->script_class.name			= "script";
	intern->script_class.flags		    = JSCLASS_GLOBAL_FLAGS;

	/* Mandatory non-null function pointer members. */
	intern->script_class.addProperty	= JS_PropertyStub;
	intern->script_class.delProperty	= JS_PropertyStub;
	intern->script_class.getProperty	= JS_PropertyStub;
	intern->script_class.setProperty	= JS_PropertyStub;
	intern->script_class.enumerate	    = JS_EnumerateStub;
	intern->script_class.resolve		= JS_ResolveStub;
	intern->script_class.convert		= JS_ConvertStub;
	intern->script_class.finalize		= JS_FinalizeStub;

	/* Optionally non-null members start here. */
	intern->script_class.getObjectOps	= 0;
	intern->script_class.checkAccess	= 0;
	intern->script_class.call			= 0;
	intern->script_class.construct		= 0;
	intern->script_class.xdrObject		= 0;
	intern->script_class.hasInstance	= 0;
	intern->script_class.mark			= 0;
	intern->script_class.reserveSlots	= 0;

	/* register global functions */
	intern->global_functions[0].name	= "write";
	intern->global_functions[0].call	= script_write;
	intern->global_functions[0].nargs	= 1;
	intern->global_functions[0].flags	= 0;
	intern->global_functions[0].extra	= 0;

	/* last element of the list need to be zeroed */
	memset(&intern->global_functions[1], 0, sizeof(JSFunctionSpec));

	/* says that our script runs in global scope */
	JS_SetOptions(intern->ct, JSOPTION_VAROBJFIX);

	/* set the error callback */
	JS_SetErrorReporter(intern->ct, reportError);
	
	/* create global object for execution */
	intern->obj = JS_NewObject(intern->ct, &intern->script_class, NULL, NULL);

	/* register globals functions */
	JS_DefineFunctions(intern->ct, intern->obj, intern->global_functions);

	/* initialize standard JS classes */
	JS_InitStandardClasses(intern->ct, intern->obj);
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
