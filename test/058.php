<?php
// Should not call __destruct if __wakeup throws an exception
// TODO: Adapt this to hhvm's garbage collection conventions, check what native serialize() does.
// TODO: Update skipif, http://hhvm.com/blog/2017/09/18/the-future-of-hhvm.html indicates __destruct() won't be called.
class Thrower {
	public $id;
	public $throws;
	public $dynamic;
	public function __construct($id, $throws = false) {
		$this->id = $id;
		$this->throws = $throws;
		$this->dynamic = "original";
	}
	public function __wakeup() {
		printf("Calling __wakeup %s\n", $this->id);
		$this->dynamic = "copy";
		if ($this->throws) {
			throw new Exception("__wakeup threw");
		}
	}

	public function __destruct() {
		printf("Calling __destruct %s dynamic=%s\n", $this->id, $this->dynamic);
	}
}
function main_native() {
	$a = new Thrower("a", true);
	$serialized = serialize($a);
	try {
		unserialize($serialized);
	} catch (Exception $e) {
		printf("Caught %s\n", $e->getMessage());
	}
	$a = null;
	print("Done a\n");
	$b = new Thrower("b", false);
	$serialized = serialize($b);
	$bCopy = unserialize($serialized);
	print("Unserialized b\n");
	var_dump($bCopy);

	$bCopy = null;
	$b = null;
	print("Done b\n");
}
function main() {
	$a = new Thrower("a", true);
	$serialized = igbinary_serialize($a);
	try {
		igbinary_unserialize($serialized);
	} catch (Exception $e) {
		printf("Caught %s\n", $e->getMessage());
	}
	$a = null;
	print("Done a\n");
	$b = new Thrower("b", false);
	$serialized = igbinary_serialize($b);
	$bCopy = igbinary_unserialize($serialized);
	print("Unserialized b\n");
	var_dump($bCopy);

	$bCopy = null;
	$b = null;
	print("Done b\n");
}
main_native();  // For comparison
echo "\nCompare with igbinary\n";
main();
