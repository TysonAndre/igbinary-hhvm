<?php
// Test serializing more strings than the capacity of the initial strings table.
function main() {
	$array = array();
	for ($i = 0; $i < 2; $i++) {
		for ($c = 'a'; $c < 'z'; $c++) {
			$array[] = $c;
		}
	}
	$serialized = igbinary_serialize($array);
	printf("%s\n", bin2hex(substr($serialized, 4)));
	$unserialized = igbinary_unserialize($serialized);
	echo implode(',', $unserialized);
}
main();
?>
