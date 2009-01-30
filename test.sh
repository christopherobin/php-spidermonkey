#!/bin/sh
php -n -d extension_dir=modules/ -d extension=spidermonkey.so tests/base.php
