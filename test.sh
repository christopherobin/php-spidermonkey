#!/bin/sh
echo "**** Running basic tests ****"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php
echo
echo "**** Running version system tests ****"
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/version.php
