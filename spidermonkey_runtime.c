#include "php_spidermonkey.h"

static int le_jsruntime_descriptor;

/**
* JSRuntime embedding
*/
zend_class_entry *php_spidermonkey_jsr_entry;

PHP_METHOD(JSRuntime, __construct)
{
    // maybe we should add an array containing options ?
//    return SUCCESS;
}

/* {{{ proto public JSContext JSRuntime::createContext()
   Returns an instance of JSContext using this runtime */
PHP_METHOD(JSRuntime, createContext)
{
    zval *retval_ptr = NULL;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *this;

    zval **params[1];
    
    MAKE_STD_ZVAL(this);
	this->type = IS_OBJECT;
	this->is_ref = 1;
	this->value.obj.handle = (getThis())->value.obj.handle;
	this->value.obj.handlers = (getThis())->value.obj.handlers;
	zval_copy_ctor(this);

    params[0] = &this;

    // init object
    object_init_ex(return_value, php_spidermonkey_jsc_entry);

    if (php_spidermonkey_jsc_entry->constructor)
    {
        fci.size = sizeof(fci);
        fci.function_table = EG(function_table);
        fci.function_name = NULL;
        fci.symbol_table = NULL;
        fci.object_pp = &return_value;
        fci.retval_ptr_ptr = &retval_ptr;
        fci.param_count = 1;
        fci.params = params;
        fci.no_separation = 1;

        fcc.initialized = 1;
        fcc.function_handler = php_spidermonkey_jsc_entry->constructor;
        fcc.calling_scope = EG(scope);
        fcc.object_pp = &return_value;

        if (zend_call_function(&fci, &fcc TSRMLS_CC) == FAILURE) {
	        if (retval_ptr) {
		        zval_ptr_dtor(&retval_ptr);
	        }
	        zend_error(E_ERROR, "Invocation of JSContext's constructor failed", php_spidermonkey_jsc_entry->name);
	        RETURN_NULL();
        }

        if (retval_ptr) {
	        zval_ptr_dtor(&retval_ptr);
        }
    }
}
/* }}} */

