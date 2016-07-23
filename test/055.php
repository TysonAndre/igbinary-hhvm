<?php
// __wakeup can replace a copy of the object referring to the root node.
class Obj {
	function __construct($a) {
		$this->a = $a;
	}

	public function __wakeup() {
		echo "Calling __wakeup\n";
		$this->a = "replaced";
	}
}

$a = new stdClass();
$a->obj = new Obj($a);;
$serialized = igbinary_serialize($a);
printf("%s\n", bin2hex($serialized));
$unserialized = igbinary_unserialize($serialized);
var_dump($unserialized);