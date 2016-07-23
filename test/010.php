<?php
// Array test

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

$a = array(
	'a' => array(
		'b' => 'c',
		'd' => 'e'
	),
	'f' => array(
		'g' => 'h'
	)
);

test('array', $a, false);