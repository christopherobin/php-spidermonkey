<?php

$a = new JSRuntime();
$b = $a->createContext();
$b->registerFunction('printf', 'printf');
$b->registerFunction('fputs', 'fputs');

$assScr = <<<SCR
node = dom.createElement("test")
dom.appendChild(node)
node2 = dom.createElement("foo", "bar")
node.appendChild(node2)
printf("%s", dom.saveXML())
fputs(stdin, "bleh ?" + PHP_EOL);
SCR;
$dom = new DOMDocument();
$b->assign('dom', $dom);
$b->assign('stdin', STDIN);
$b->assign('PHP_EOL', PHP_EOL);
$b->evaluateScript($assScr);

//echo $dom->saveXML();
