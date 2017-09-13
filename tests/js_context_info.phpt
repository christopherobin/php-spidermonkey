--TEST--
Check JSContext->getVersionString()
--FILE--
<?php
$js = new JSContext();
echo $js->getVersionString(160);

?>
--EXPECTF--
1.6