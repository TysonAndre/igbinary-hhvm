<?php
// Object test, incomplete class

function test($type, $variable, $test) {
	$serialized = pack('H*', $variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	var_dump($unserialized);
//	echo "\n";
}

test('incom', '0000000217034f626a140211016106011101620602', false);