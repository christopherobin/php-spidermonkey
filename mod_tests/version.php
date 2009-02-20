<?php

$script1 = <<<SCR
var a = [ 7, 15, 789, 324, 48, 23 ];
a.map(function(i) { return i*i }).toString()
SCR;

$b = new JSContext();
if (!$b->setVersion(JSVERSION_1_5))
	echo "couldn't set context version to " . $b->getVersionString(JSVERSION_1_5) . "\n";
echo "Using array map function ( JS " . $b->getVersionString($b->getVersion()) . " ):\n";
var_dump($b->evaluateScript($script1));

