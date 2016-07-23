<?php
// Cyclic array test
function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo !$test || $unserialized == $variable ? 'OK' : 'ERROR', "\n";
}

$a = array(
	'a' => array(
		'b' => 'c',
		'd' => 'e'
	),
);

$a['f'] = &$a;

test('array', $a, false);

$a = array("foo" => &$b);
$b = array(1, 2, $a);

$exp = $a;
$act = igbinary_unserialize(igbinary_serialize($a));

ob_start();
var_dump($exp);
$dump_exp = ob_get_clean();
ob_start();
var_dump($act);
$dump_act = ob_get_clean();

if ($dump_act !== $dump_exp) {
	echo "Var dump differs:\n", $dump_act, "\n", $dump_exp, "\n";
} else {
	echo "Var dump OK\n";
}

$act['foo'][1] = 'test value';
$exp['foo'][1] = 'test value';
if ($act['foo'][1] !== $act['foo'][2]['foo'][1]) {
	echo "Recursive elements differ:\n";
	var_dump($act);
	var_dump($act['foo']);
	var_dump($exp);
	var_dump($exp['foo']);
}