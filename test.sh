#!/bin/sh
echo "**** Running basic tests ****"
echo "php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php
echo
echo "**** Running version system tests ****"
echo "php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php
