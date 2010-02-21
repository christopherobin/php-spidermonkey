--TEST--
Regression test for bug 16876 ( Empty string as a parameter causes segfault )
--FILE--
<?php
class myClass
{
   public function test( $str )
   {
      //any code
      echo 'TestWrite';
   }
}

$myclass = new myClass();

$js = new JSContext();
$js->assign("myclass", $myclass);

$js->evaluateScript( 'myclass.test("")' );
?>
--EXPECTF--
TestWrite
