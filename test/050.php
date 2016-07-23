<?php
// Correctly unserialize cyclic object references
$a = new stdClass();
$a->foo = &$a;
$a->bar = &$a;
$b = new stdClass();
$b->cyclic = &$a;
printf("%s\n", serialize($b));
$ig_ser = igbinary_serialize($b);
printf("%s\n", bin2hex($ig_ser));
$ig = igbinary_unserialize($ig_ser);
printf("%s\n", serialize($ig));
var_dump($ig);
$f = &$ig->cyclic->foo;
$f = 'V';
var_dump($ig);
// Note: While the php7 unserializer consistently makes a distinction between refs to an object and non-refs,
// the php5 serializer does not yet.