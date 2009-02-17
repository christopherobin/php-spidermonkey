<?php

$a = new JSRuntime();
$b = $a->createContext();

function test_callback()
{
	var_dump(func_get_args());
}

$b->registerFunction('vroum', 'test_callback');
$b->evaluateScript('vroum(1567, "string", 12.7, false, { a : 17, b : "foo" })');
