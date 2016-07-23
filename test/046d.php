<?php
// Correctly unserialize multiple object refs and non-refs.
$a = array(new stdClass());
$a[1] = $a[0];
$a[2] = &$a[1];
$a[3] = $a[0];
var_dump($a);
printf("%s\n", serialize($a));
$ig_ser = igbinary_serialize($a);
printf("%s\n", bin2hex($ig_ser));
$ig = igbinary_unserialize($ig_ser);
printf("%s\n", serialize($ig));
var_dump($ig);
$f = &$ig[2];
$f = 'V';
var_dump($ig);
// Note: While the php7 unserializer consistently makes a distinction between refs to an object and non-refs,
// the php5 serializer does not yet.