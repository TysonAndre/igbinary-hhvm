<?php
// Object-Reference test

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR', "\n";
	if ($unserialized != $variable) {
		var_dump($variable, $unserialized);
	}
	echo "\n";
}

class Obj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}
}

$o = new Obj(1, 2);
$a = array(&$o, &$o);

test('object', $a, false);
