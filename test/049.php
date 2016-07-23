<?php
// Correctly unserialize multiple references in arrays
class Foo{}
$a = array("A");
$a[1] = &$a[0];
$a[2] = &$a[1];
$a[3] = &$a[2];
$a[4] = false;
$a[5] = &$a[4];
$a[6] = new Foo();
$a[7] = &$a[6];
$a[8] = &$a[7];
$a[9] = [33];
$a[10] = new stdClass();
$a[10]->prop = &$a[8];
$a[11] = &$a[10];
$a[12] = $a[9];
$ig_ser = igbinary_serialize($a);
printf("ig_ser=%s\n", bin2hex($ig_ser));
$ig = igbinary_unserialize($ig_ser);
var_dump($ig);
$f = &$ig[3];
$f = 'V';
$g = &$ig[5];
$g = 'H';
$h = $ig[10];
$h->prop = 'S';
var_dump($ig);