--TEST--
Check JSContext->evaluete()
--FILE--
<?php
$js = new JSContext();
$js->evaluateScript('var a = 1;');
echo $js->evaluateScript('a');

?>
--EXPECTF--
1