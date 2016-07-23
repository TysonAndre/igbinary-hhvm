<?php
// Object test, unserialize_callback_func

function test($type, $variable, $test) {
	$serialized = pack('H*', $variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized->b == 2 ? 'OK' : 'ERROR';
	echo "\n";
}

function autoload($classname) {
	class Obj {
		var $a;
		var $b;

		function __construct($a, $b) {
			$this->a = $a;
			$this->b = $b;
		}
	}
}

test('autoload', '0000000217034f626a140211016106011101620602', false);