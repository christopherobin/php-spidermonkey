<?php

$script1 = <<<SCR
var MyMath = { hyp : function(a, b) { return Math.sqrt(a*a + b*b) } }
MyMath.hyp(7, 15)
SCR;

$script2 = <<<SCR
var MyConcat = function(a, b) { return a + ' ' + b }
MyConcat("hello", "world")
SCR;

$a = new JSRuntime();
$b = new JSContext($a);
$c = new JSObject($b);
$d = new JSObject($b);
echo "MyMath.hyp(7, 15):\n";
var_dump($c->evaluateScript($script1));
echo "MyConcat(\"hello\", \"world\")\n";
var_dump($d->evaluateScript($script2));

