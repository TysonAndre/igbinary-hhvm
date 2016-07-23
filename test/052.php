<?php
// Object Serializable interface can be serialized in references

function test($variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
}

class Obj implements Serializable {
	private static $count = 1;

	public $a;
	public $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	public function serialize() {
		$c = self::$count++;
		echo "call serialize\n";
		return pack('NN', $this->a, $this->b);
	}

	public function unserialize($serialized) {
		$tmp = unpack('N*', $serialized);
		$this->__construct($tmp[1], $tmp[2]);
		$c = self::$count++;
		echo "call unserialize\n";
	}
}

function main() {
	$a = new Obj(1, 0);
	$b = new Obj(42, 43);
	$variable = [&$a, &$a, $b];
	$serialized = igbinary_serialize($variable);
	printf("%s\n", bin2hex($serialized));
	$unserialized = igbinary_unserialize($serialized);
	var_dump($unserialized);
	$unserialized[0] = 'A';
	var_dump($unserialized[1]);
}
main();