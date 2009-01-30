#!/bin/sh
php -n -d extension_dir=modules/ -d extension=spidermonkey.so -r 'var_dump(memory_get_usage(true)); $a = new JSRuntime(); $b = new JSContext($a); var_dump(memory_get_usage(true));'
