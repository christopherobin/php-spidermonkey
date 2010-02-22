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

$js->evaluateScript($script);
?>
--EXPECTF--
<?xml version="1.0"?>
<foo>
  <bar bleh="blah">meh</bar>
</foo>

Fatal error: Uncaught exception 'Exception' with message 'Trying to reference global object' in /home/bomb/Desktop/Code/C/spidermonkey/tests/js_objects.php:25
Stack trace:
#0 /home/bomb/Desktop/Code/C/spidermonkey/tests/js_objects.php(25): JSContext->evaluateScript('dom = new DOMDo...')
#1 {main}
  thrown in /home/bomb/Desktop/Code/C/spidermonkey/tests/js_objects.php on line 25
