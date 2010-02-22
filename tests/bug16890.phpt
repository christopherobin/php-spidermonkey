--TEST--
Regression test for bug 16890 ( Variable is cleaned while being used in JS )
--FILE--
<?php

function test($arr) {
    var_dump($arr);
}

$ctx = new JSContext();
$ctx->registerFunction('test');

$js = <<<JS
arr = [1, null, 0, 'str'];
test(arr);
test(arr);

a = { b : null };
a.b = a;

test(a);
JS;

$ctx->evaluateScript($js);
?>
--EXPECTF--
object(stdClass)#2 (4) {
  ["0"]=>
  int(1)
  ["1"]=>
  NULL
  ["2"]=>
  int(0)
  ["3"]=>
  string(3) "str"
}
object(stdClass)#2 (4) {
  ["0"]=>
  int(1)
  ["1"]=>
  NULL
  ["2"]=>
  int(0)
  ["3"]=>
  string(3) "str"
}
object(stdClass)#2 (1) {
  ["b"]=>
  object(stdClass)#2 (1) {
    ["b"]=>
    *RECURSION*
  }
}
