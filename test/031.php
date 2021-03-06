<?php
// Object Serializable interface throws exceptions

function test($variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
}

class Obj implements Serializable {
	private static $count = 1;

	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	public function serialize() {
		$c = self::$count++;
		echo "call serialize, ", ($this->a ? "throw" : "no throw"),"\n";
		if ($this->a) {
			throw new Exception("exception in serialize $c");
		}
		return pack('NN', $this->a, $this->b);
	}

	public function unserialize($serialized) {
		$tmp = unpack('N*', $serialized);
		$this->__construct($tmp[1], $tmp[2]);
		$c = self::$count++;
		echo "call unserialize, ", ($this->b ? "throw" : "no throw"),"\n";
		if ($this->b) {
			throw new Exception("exception in unserialize $c");
		}
	}
}

$a = new Obj(1, 0);
$a = new Obj(0, 0);
$b = new Obj(0, 0);
$c = new Obj(1, 0);
$d = new Obj(0, 1);

echo "a, a, c\n";
try {
	test(array($a, $a, $c));
} catch (Exception $e) {
    if ($e->getPrevious()) {
        $e = $e->getPrevious();
    }

	echo $e->getMessage(), "\n";
}

echo "b, b, d\n";

try {
	$result = test(array($b, $b, $d));
    echo "Unexpected result\n";
    var_export($result);
} catch (Exception $e) {
    if ($e->getPrevious()) {
        $e = $e->getPrevious();
    }

	echo $e->getMessage(), "\n";
}
echo "Done\n";
