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
/* $b = new JSContext($a); */
$b = $a->createContext();
/* $c = new JSObject($b); */
$c = $b->createObject();
echo "MyMath.hyp(7, 15):\n";
var_dump($c->evaluateScript($script1));
echo "MyConcat(\"hello\", \"world\"):\n";
var_dump($c->evaluateScript($script2));
echo "ParseInt(MyMath.hyp(4, 6)):\n";
var_dump($c->evaluateScript("parseInt(MyMath.hyp(4, 6))"));
var_dump($c->evaluateScript('write("hello world !\n")'));
