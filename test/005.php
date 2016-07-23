<?php
// Check for double serialisation

function test($type, $variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $unserialized === $variable ? 'OK' : 'ERROR';
	echo "\n";
}

test('double: 123.456', 123.456);
