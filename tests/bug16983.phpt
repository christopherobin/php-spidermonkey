--TEST--
Regression test for bug 16983 ( Problems with registering classes containing __get() )
--FILE--
<?php
$ctx = new JSContext();
$ctx->registerFunction("var_dump");

class Test {
    public $foo = "bar";
    
	public function __get($key){
		// Problem exists even if not used, it is enough if it is defined! 
		// If you remove this method definition, everything works fine.
        $this->foo = "bar2";
        return "bleh";
	}
	
    public function doSomething(){
    	echo "Foo!\n";
    }
}

$ctx->registerClass("Test");
$ctx->evaluateScript("var m = new Test(); m.doSomething(); var_dump(m.test); var_dump(m.foo);");
?>
--EXPECTF--
Foo!
string(4) "bleh"
string(4) "bar2"
