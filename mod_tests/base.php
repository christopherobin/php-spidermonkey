<?php

$script1 = <<<SCR
var MyMath = { hyp : function(a, b) { return Math.sqrt(a*a + b*b) } }
MyMath.hyp(7, 15)
SCR;

$script2 = <<<SCR
var MyConcat = function(a, b) { return a + ' ' + b }
MyConcat("hello", "world")
SCR;

$script3 = <<<SCR
obj = {
	name : "test",
	data : {
		id      : "vroum",
		content : 524,
		value   : 1567.15,
		a_bool  : false,
		some_rgx: /^<strong>.*?<\/strong>/
	}
}
SCR;

$a = new JSRuntime();
$b = $a->createContext();
echo "MyMath.hyp(7, 15):\n";
var_dump($b->evaluateScript($script1));
echo "MyConcat(\"hello\", \"world\"):\n";
var_dump($b->evaluateScript($script2));
echo "ParseInt(MyMath.hyp(4, 6)):\n";
var_dump($b->evaluateScript("parseInt(MyMath.hyp(4, 6))"));
var_dump($b->evaluateScript('write("hello world !\n")'));
var_dump($b->evaluateScript($script3));
