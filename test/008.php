<?php

// Check for array+string serialization
function test($type, $variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	if ($unserialized != $variable) {
		echo 'ERROR, expected: ';
		var_dump($variable);
		echo 'got: ';
		var_dump($unserialized);
	} else {
		echo 'OK';
	}
	echo "\n";
}

test('array("foo", "foo", "foo")', array("foo", "foo", "foo"));
test('array("one" => 1, "two" => 2))', array("one" => 1, "two" => 2));
test('array("kek" => "lol", "lol" => "kek")', array("kek" => "lol", "lol" => "kek"));
test('array("" => "empty")', array("" => "empty"));
