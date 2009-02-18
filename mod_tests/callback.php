<?php

$a = new JSRuntime();
$b = $a->createContext();
$b->registerFunction('printf', 'printf');
$b->registerFunction('read_file', 'file_get_contents');

$assScr = <<<SCR
content = read_file('test.xml')
dom.loadXML(content)
printf("%s", dom.saveXML())
SCR;
$dom = new DOMDocument();
$b->assign('dom', $dom);
$b->evaluateScript($assScr);

//echo $dom->saveXML();
