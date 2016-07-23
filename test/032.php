<?php
// Object test, __sleep and __wakeup exceptions
function test($variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
}

class Obj {
	private static $count = 0;
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		$c = self::$count++;
		if ($this->a) {
			throw new Exception("exception in __sleep $c");
		}
		return array('a', 'b');
	}

	function __wakeup() {
		$c = self::$count++;
		if ($this->b) {
			throw new Exception("exception in __wakeup $c");
		}
		$this->b = $this->a * 3;
	}
}


$a = new Obj(1, 0);
$b = new Obj(0, 1);
$c = new Obj(0, 0);

try {
	test($a);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

try {
	test($b);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

try {
	test($c);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}