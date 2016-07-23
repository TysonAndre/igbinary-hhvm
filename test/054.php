<?php
// __wakeup can add dynamic properties without affecting other objects
class Obj {
	// Testing $this->a being a dynamic property.

	function __construct($a) {
		$this->a = $a;
	}

	public function __wakeup() {
		echo "Calling __wakeup\n";
		for ($i = 0; $i < 10000; $i++) {
			$this->{'b' . $i} = 42;
		}
	}
}

function main() {
	$array = ["roh"];  // array (not a reference, but should be copied on write)
	$a = new Obj($array);
	$b = new Obj($array);
	$c = new Obj(null);
	$variable = [$a, $b, $c];
	$serialized = igbinary_serialize($variable);
	printf("%s\n", bin2hex($serialized));
	$unserialized = igbinary_unserialize($serialized);
	echo "Called igbinary_unserialize\n";
	for ($a = 0; $a < 3; $a++) {
		for ($i = 0; $i < 10000; $i++) {
			if ($unserialized[$a]->{'b' . $i} !== 42) {
				echo "Fail $a b$i\n";
				return;
			}
			unset($unserialized[$a]->{'b' . $i});
		}
	}
	var_dump($unserialized);
}
main();