--TEST--
basic test for conversion JS <-> PHP
--FILE--
<?php
$js = new JSContext();

$a = array(
	'str'	=> 'a string',
	'long'	=> 1337,
	'flt'	=> 13.37,
	'bool'	=> false,
	'null'	=> null,
	'numarr'=> array(1, 2, 3, 5, 7, 11, 13, 17)
);

$js->assign('test', $a);

var_dump($js->evaluateScript('test'));
?>
--EXPECTF--
object(stdClass)#2 (6) {
  ["str"]=>
  string(8) "a string"
  ["long"]=>
  int(1337)
  ["flt"]=>
  float(13.37)
  ["bool"]=>
  bool(false)
  ["null"]=>
  NULL
  ["numarr"]=>
  object(stdClass)#3 (8) {
    ["0"]=>
    int(1)
    ["1"]=>
    int(2)
    ["2"]=>
    int(3)
    ["3"]=>
    int(5)
    ["4"]=>
    int(7)
    ["5"]=>
    int(11)
    ["6"]=>
    int(13)
    ["7"]=>
    int(17)
  }
}
