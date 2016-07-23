<?php
// Check for double NaN, Inf, -Inf, 0, and -0. IEEE 754 doubles
function str2bin($bytestring) {
	$len = strlen($bytestring);
	$output = '';

	for ($i = 0; $i < $len; $i++) {
		$bin = decbin(ord($bytestring[$i]));
		$bin = str_pad($bin, 8, '0', STR_PAD_LEFT);
		$output .= $bin;
	}

	return $output;
}

function test($type, $variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, ":\n";
	var_dump($variable);
	var_dump($unserialized);

	echo "   6         5         4         3         2         1          \n";
	echo "3210987654321098765432109876543210987654321098765432109876543210\n";
	echo str2bin(substr($serialized, 5, 8)), "\n";
	echo "\n";
}

// exponent all-1, non zero mantissa
test('double NaN', NAN);

// sign 0, exp all-1, zero mantissa
test('double Inf', INF);

// sign 1, exp all-1, zero mantissa
test('double -Inf', -INF);

// sign 0, all-0
test('double 0.0', 0.0);

// sign 1, all-0
test('double -0.0', -1 * 0.0);
