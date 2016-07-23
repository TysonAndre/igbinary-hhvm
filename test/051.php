<?php
// Object test, __wakeup (With multiple references)

class Obj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __wakeup() {
		$this->b = $this->a * 3;
	}
}

function main() {
	$o = new Obj(1, 2);
	$variable = [&$o, &$o];
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo substr(bin2hex($serialized), 8), "\n";
	echo $unserialized[0]->b === 3 && $unserialized[0]->a === 1 ? 'OK' : 'ERROR';
	echo "\n";
	$unserialized[0] = 'a';
	var_dump($unserialized);
}

main();