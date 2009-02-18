<?php

$a = new JSRuntime();
$b = $a->createContext();
$b->registerFunction('printf', 'printf');
$b->registerFunction('read_file', 'file_get_contents');

$assScr = <<<SCR
content = read_file('test.xml')
dom.loadXML(content)
/*printf("%s", content)*/
SCR;
$dom = new DOMDocument();
//$b->assign('env', $_ENV);
$b->assign('test_xml', array($dom, 'loadXML'));
$b->assign('dom', $dom);
$b->evaluateScript($assScr);
