<?php
// Correctly unserialize multiple references in objects
class Foo{}
$a = new stdClass();
$a->x0 = NULL;
$a->x1 = &$a->x0;
$a->x2 = &$a->x1;
$a->x3 = &$a->x2;
$a->x4 = false;
$a->x5 = &$a->x4;
$a->x6 = new Foo();
$a->x7 = &$a->x6;
$a->x8 = &$a->x7;
$a->x9 = [33];
$a->x10 = new stdClass();
$a->x10->prop = &$a->x8;
$a->x11 = &$a->x10;
$a->x12 = $a->x9;
$ig_ser = igbinary_serialize($a);
printf("ig_ser=%s\n", bin2hex($ig_ser));
$ig = igbinary_unserialize($ig_ser);
$f = &$ig->x3;
$f = 'V';
$g = &$ig->x5;
$g = 'H';
$h = $ig->x10;
$h->prop = 'S';
var_dump($ig);