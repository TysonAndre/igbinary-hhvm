<?php
// Check for bool serialisation
function test($type, $variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $unserialized === $variable ? 'OK' : 'ERROR';
	echo "\n";
}

test('bool true',  true);
test('bool false', false);
