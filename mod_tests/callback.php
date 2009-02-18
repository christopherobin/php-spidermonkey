<?php

$a = new JSRuntime();
$b = $a->createContext();
$b->registerFunction('printf', 'printf');

$assScr = <<<SCR
node = dom.createElement("test")
dom.appendChild(node)
node2 = dom.createElement("foo", "bar")
node.appendChild(node2)
printf("%s", dom.saveXML())
SCR;
$dom = new DOMDocument();
$b->assign('dom', $dom);
$b->evaluateScript($assScr);

//echo $dom->saveXML();
