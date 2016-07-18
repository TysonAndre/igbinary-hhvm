<?php
function test($type, $variable) {

	echo $type, "\n";
	$serialized = igbinary_serialize($variable);

	echo substr(bin2hex($serialized), 8), "\n";

	$unserialized = igbinary_unserialize($serialized);
	echo $unserialized === $variable ? 'OK' : 'ERROR';
	echo "\n";
}

test('null', null);
