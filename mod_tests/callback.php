<?php

$b = new JSContext();
$b->registerFunction('printf', 'printf');
$b->registerFunction('fputs', 'fputs');
$b->registerFunction('curl_init', 'curl_init');
$b->registerFunction('curl_exec', 'curl_exec');

$script = <<<SCR
printf('******** OBJECT MANAGEMENT TEST ********' + PHP_EOL)
node = dom.createElement("test")
dom.appendChild(node)
node2 = dom.createElement("foo", "bar")
node.appendChild(node2)
printf("%s", dom.saveXML())
printf(PHP_EOL + '******** BASIC RESOURCE TEST ********' + PHP_EOL)
fputs(stdin, "bleh ?" + PHP_EOL);
printf(PHP_EOL + '******** COMPLEX RESOURCE TEST ********' + PHP_EOL)
ch = curl_init('http://wimip.fr/?t=');
curl_exec(ch);
printf(PHP_EOL);
SCR;
$dom = new DOMDocument();
$b->assign('dom', $dom);
$b->assign('stdin', STDIN);
$b->assign('PHP_EOL', PHP_EOL);
$b->evaluateScript($script);

