<?php
// Object test, __sleep error cases

error_reporting(0);

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

class Obj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		return array('c');
	}

#	function __wakeup() {
#		$this->b = $this->a * 3;
#	}
}

class Opj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		return array(1);
	}

#	function __wakeup() {
#
#	}
}

$o = new Obj(1, 2);
$p = new Opj(1, 2);

test('nonexisting', $o, true);
test('wrong', $p, true);