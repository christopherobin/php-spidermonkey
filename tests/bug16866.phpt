--TEST--
Regression test for bug 16866 ( Returning empty string from static function causes segmentation fault )
--FILE--
<?php
$app = new App();
$app->run();

class App {	
	public function run () {
		$context = new JSContext();
		$context->registerClass( 'TimeObject' );
		echo $context->evaluateScript( 'var t = new TimeObject();
t.getTime();' );
	}
	
	static function getString() {
		return ''; // <-- empty string causes seg fault
		//return ' '; // <-- a space won't cause seg fault
	}
}

class TimeObject {
	public function getTime () {
		return App::getString();
	}
}
?>
--EXPECTF--

