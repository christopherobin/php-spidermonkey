<?php

$b = new JSContext();
$b->registerFunction('printf');
$b->registerFunction('fputs');
$b->registerFunction('fopen');
$b->registerFunction('fgets');
$b->registerFunction('feof');
$b->registerFunction('var_dump');
$b->registerClass('DOMDocument');

$file = __FILE__;

$script = <<<SCR
printf('******** OBJECT MANAGEMENT TEST ********' + PHP_EOL)

dom = new DOMDocument()
dom.formatOutput = true
dom.bleh = 17
node = dom.createElement("test")
dom.appendChild(node)
node2 = dom.createElement("foo", "bar")
node.appendChild(node2)
printf("%s", dom.saveXML())

printf(PHP_EOL + '******** BASIC RESOURCE TEST ********' + PHP_EOL)

fputs(stdin, "bleh ?" + PHP_EOL);

printf(PHP_EOL + '******** COMPLEX RESOURCE TEST ********' + PHP_EOL)

ch = fopen('http://wimip.fr/?t=', 'r');
if (ch) {
	ch.read = function() { return fgets(this) }
	while (data = ch.read()) {
		printf(data)
	}
}
printf(PHP_EOL);
SCR;
$b->assign('stdin', STDIN);
$b->assign('PHP_EOL', PHP_EOL);
$b->evaluateScript($script);

