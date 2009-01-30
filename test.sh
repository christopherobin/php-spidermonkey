#!/bin/sh
php -n -d extension_dir=modules/ -d extension=spidermonkey.so mod_tests/base.php
