--TEST--
Regression test for bug 22838 ( Segfault when calling exit from PHP )
--FILE--
<?php
echo "start\n";
$ctx = new JSContext();
echo "finished\n";
exit;
?>
--EXPECTF--
start
finished

