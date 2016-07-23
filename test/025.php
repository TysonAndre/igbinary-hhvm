<?php
// Object test, array of objects with __sleep
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	var_dump($variable);
	var_dump($unserialized);
}

class Obj {
	public $a;
	protected $b;
	private $c;
	var $d;

	function __construct($a, $b, $c, $d) {
		$this->a = $a;
		$this->b = $b;
		$this->c = $c;
		$this->d = $d;
	}

	function __sleep() {
		return array('a', 'b', 'c');
	}
}

$array = array(
	new Obj("aa", "bb", "cc", "dd"),
	new Obj("ee", "ff", "gg", "hh"),
	new Obj(1, 2, 3, 4),
);

test('array', $array, true);