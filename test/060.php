<?php
// Igbinary_unserialize_header warning
function my_error_handler($errno, $errstr) {
	printf("Logged: $errstr\n");
}
error_reporting(E_ALL);
set_error_handler('my_error_handler');

function test_igbinary_unserialize($data) {
	$unserialized = igbinary_unserialize($data);
	if ($unserialized !== null) {
		printf("Expected null, got %s\n", var_export($unserialized, true));
	}
}

test_igbinary_unserialize("a:0:{}");
test_igbinary_unserialize('\\""\\{}');
test_igbinary_unserialize("\x00\x00\x00\x01");
test_igbinary_unserialize("\x00\x00\x00\x03\x00");
test_igbinary_unserialize("\x00\x00\x00\xff\x00");
test_igbinary_unserialize("\x02\x00\x00\x00\x00");
