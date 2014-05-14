php-spidermonkey
================

Interface for using spidermonkey in PHP

Installation
============

Make sure you have a recent version of spidermonkey installed (JSAPI 17 or more).
Then checkout that project, and inside the folder run:

```sh
phpize
./configure
make
```

You should find the **spidermonkey.so** file in the *modules* folder.

Usage
=====

```php
<?php

// a JSContext is where your code run, each context has it's own scope
$js = new JSContext();

// you can register functions to be made availables in js
$js->registerFunction('printf');
// by default it makes it available as the same name, you can change that by doing
$js->registerFunction('printf', 'another_name');

// you can also register PHP classes for access, doing so will make that class
// available for instanciation in javascript and all methods available
$js->registerClass('mysqli');

// then to run you actual script, just do the following
try {
    // script is a string containing javascript code
    $js->evaluateScript($script);
} catch (Exception $e) {
    // do something with the exception
}
```
