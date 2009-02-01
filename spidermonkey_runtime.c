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

PHP_METHOD(JSRuntime, createContext)
{
    // do something

}

