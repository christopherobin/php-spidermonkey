#!/bin/sh

PHP="php"

if [ x"$1" != x ]; then
	PHP="$1";
fi;

echo "**** Running basic tests ****"
echo "$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php"
$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php
echo
echo "**** Running version system tests ****"
echo "$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php"
$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php
echo
echo "**** Running callback system tests ****"
echo "$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/callback.php"
$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/callback.php
#echo
#echo "**** Running reference system tests ****"
#echo "$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/reference.php"
#$PHP -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/reference.php
