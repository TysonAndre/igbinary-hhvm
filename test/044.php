<?php
// Check for double extremes
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

// subnormal number
test('double subnormal', -4.944584125e-314);

// max subnormal: sign 0, exponent 0, all 1 double
// http://www.exploringbinary.com/php-hangs-on-numeric-value-2-2250738585072011e-308/
test('double 1 max subnormal', 2.2250738585072010e-308);
test('double 2 max subnormal', 2.2250738585072011e-308);
test('double 3 max subnormal', 2.2250738585072012e-308);
test('double 4 max subnormal', 2.2250738585072013e-308);
test('double 5 max subnormal', 2.2250738585072014e-308);

// min subnormal number
test('double min subnormal', -4.9406564584124654e-324);

// big double
test('double big', -1.79769e308);

// max double, sign 0, exponent all-1 - 1, mantissa all-1
test('double max',  1.7976931348623157e308);

// small double
test('double small', -2.225e-308);
// min double, sign 1, exponent all-1 - 1, mantissa all-1
test('double min',  -1.7976931348623157e308);
