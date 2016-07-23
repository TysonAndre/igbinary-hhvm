<?php
// Correctly unserialize scalar refs.
$a = array(13);
$a[1] = &$a[0];
$a[2] = &$a[1];
$a[3] = &$a[2];

$ig_ser = igbinary_serialize($a);
$ig = igbinary_unserialize($ig_ser);
$f = &$ig[3];
$f = 'V';
var_dump($ig);