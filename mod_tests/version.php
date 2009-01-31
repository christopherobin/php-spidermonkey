<?php

$script1 = <<<SCR
var a = [ 7, 15, 789, 324, 48, 23 ];
a.map(function(i) { return i*i }).toString()
SCR;

$a = new JSRuntime();
$b = new JSContext($a);
if (!$b->setVersion(JSVERSION_1_5))
	echo "couldn't set context version to " . $b->getVersionString(JSVERSION_1_5) . "\n";
$c = new JSObject($b);
echo "Using array map function ( JS " . $b->getVersionString($b->getVersion()) . " ):\n";
var_dump($c->evaluateScript($script1));

