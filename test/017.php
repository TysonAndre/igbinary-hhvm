<?php
// Object test, __wakeup

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized->b == 3 ? 'OK' : 'ERROR';
	echo "\n";
}

class Obj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __wakeup() {
		$this->b = $this->a * 3;
	}
}

$o = new Obj(1, 2);

test('object', $o, false);