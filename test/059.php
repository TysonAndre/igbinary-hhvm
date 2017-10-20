<?php
// igbinary_unserialize should never convert from packed array to hash when references exist (Bug #48)
function main() {
	$result = array();
	// The default hash size is 16. If we start with 0, and aren't careful, the array would begin as a packed array,
	// and the references would be invalidated when key 50 (>= 16) is added (converted to a hash), causing a segfault.
	foreach (array(0, 50) as $i) {
	    $inner = new stdClass();
	    $inner->a = $i;
	    $result[0][0][$i] = $inner;
	    $result[1][] = $inner;
	}
	$serialized = igbinary_serialize($result);
	printf("%s\n", bin2hex(substr($serialized, 4)));
	flush();
	$unserialized = igbinary_unserialize($serialized);
	var_dump($unserialized);
}
main();
