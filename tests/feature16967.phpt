--TEST--
Feature request 16967 ( Export javascript functions from Javascript to PHP )
--FILE--
<?php

$js     = new JSContext;
$window = new stdClass;
$window->foo = function() { echo "bar"; };

$js->assign('window', $window);

$script = <<<JS
	window.foo();

    window.onload = function () {
        return 'foobar';
    };

    window.onload();
JS;

var_dump($js->evaluateScript($script));
var_dump($window->onload instanceof Closure);
var_dump($window->onload);
?>
--EXPECTF--
string(6) "foobar"
bool(true)
object(Closure)#3 (0) {
}
