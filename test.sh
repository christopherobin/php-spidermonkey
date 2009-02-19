#!/bin/sh
echo "**** Running basic tests ****"
echo "php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php
echo
echo "**** Running version system tests ****"
echo "php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php
echo
echo "**** Running callback system tests ****"
echo "php -n -d extension_dir=modules/ -d extension=spidermonkey.so -d extension=curl.so mod_tests/callback.php"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so -d extension=curl.so mod_tests/callback.php
