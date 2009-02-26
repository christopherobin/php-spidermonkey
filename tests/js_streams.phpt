--TEST--
register callbacks and closures in JS
--FILE--
<?php
$js = new JSContext();
$js->registerFunction(function($str) { print($str . "\n"); }, 'print');
$js->registerFunction('tmpfile');
$js->registerFunction('fopen');
$js->registerFunction('fclose');


$file = __FILE__;
$script = <<<SCR
/* test for write, seek and read on tmpfile  */
fd = tmpfile()
fd.seek(1024, fd.SEEK_SET)
fd.write('** FOO **')
fd.seek(-6, fd.SEEK_CUR)
foo = fd.read(3)
fclose(fd)
print(foo)

/* test for getline */
fd = fopen('{$file}', 'r')
fd.getline()
line = fd.getline()
fclose(fd)
print(line)
SCR;

$js->evaluateScript($script);
?>
--EXPECTF--
FOO
$js = new JSContext();

