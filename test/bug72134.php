<?php
class Obj
{
    public $bar = "test";
}
$value = new Obj();
$value->i = 1;
$igb = igbinary_serialize($value);
for ($i=0; $i < 30; $i++)
{
    // This used to segfault at the third attempt
    echo igbinary_unserialize($igb)->bar . PHP_EOL;
}
