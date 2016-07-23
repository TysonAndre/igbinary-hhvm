<?php
// APC serializer registration
echo ini_get('apc.serializer'), "\n";

class Bar {
	public $foo = 10;
}

$a = new Bar;
apc_store('foo', $a);
unset($a);

var_dump(apc_fetch('foo'));