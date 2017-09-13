--TEST--
register callbacks and closures in JS
--FILE--
<?php

function a() {
	echo "foo";
}

class C {
	function F($param) {
		echo "{$param}\n";
	}
}

function cl() { echo "bar\n"; }

$js = new JSContext();
$js->registerFunction('printf');
$js->registerFunction('a', 'b');
$js->registerFunction(array(new C(), 'F') , 'CF');
$js->registerFunction('cl' , 'closure');

$script = <<<SCR
printf("%s %d ", "blah", 42)
b()
closure()
CF("meh")
SCR;

$js->evaluateScript($script);
?>
--EXPECTF--
blah 42 foobar
meh

