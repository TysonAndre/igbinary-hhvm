<?php
// __wakeup can modify properties without affecting other objects
class Obj {
	private static $count = 1;

	public $a;

	function __construct($a) {
		$this->a = $a;
	}

	public function __wakeup() {
		echo "call wakeup\n";
		$this->a[] = "end";
	}
}

function main() {
	$array = ["test"];  // array (not a reference, but should be copied on write)
	$a = new Obj($array);
	$b = new Obj($array);
	$variable = [$a, $b];
	$serialized = igbinary_serialize($variable);
	printf("%s\n", bin2hex($serialized));
	$unserialized = igbinary_unserialize($serialized);
	var_dump($unserialized);
}
main();