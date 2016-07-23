<?php
// Correctly unserialize multiple object refs.
$a = array(new stdClass());
$a[1] = &$a[0];
$a[2] = &$a[1];
$a[3] = &$a[2];
printf("%s\n", serialize($a));
$ig_ser = igbinary_serialize($a);
printf("%s\n", bin2hex($ig_ser));
$ig = igbinary_unserialize($ig_ser);
printf("%s\n", serialize($ig));
$f = &$ig[3];
$f = 'V';
var_dump($ig);