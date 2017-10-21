--TEST--
complex usage of objects/classes in JS
--FILE--
<?php
$js = new JSContext();
$js->registerFunction('printf');
$js->registerClass('DOMDocument');

$script = <<<SCR
dom = new DOMDocument()
dom.formatOutput = true

foo = dom.createElement('foo')
bar = dom.createElement('bar', 'meh')

bar.setAttribute('bleh', 'blah')

dom.appendChild(foo).appendChild(bar)

printf(dom.saveXML())

a = {
  b : this
};

SCR;

try {
	$js->evaluateScript($script);
} catch (Exception $e) {
	var_dump($e->getMessage());
}
?>
--EXPECTF--
<?xml version="1.0"?>
<foo><bar bleh="blah">meh</bar></foo>
string(33) "Trying to reference global object"
