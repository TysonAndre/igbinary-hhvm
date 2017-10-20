<?php
// Works when there are hash collisions in strings when serializing.
// (Not applicable to HHVM, igbinary isn't using a custom hash library)

class Fy{
    public $EzFy = 2;
    public function __construct($x) {
        $this->x = $x;
    }
}
class Ez {
    public $FyEz = 'EzEz';
}

class G8 {
    public $FyG8;
}

$data = array(new Fy('G8G8'), new Fy('EzG8'), new Ez(), new G8(), new Ez(), 'G8' => new G8(), 'F8Ez' => new G8(), array(new G8()));
var_dump($data);
echo "\n";
$str = igbinary_serialize($data);
echo bin2hex($str) . "\n";
$unserialized = igbinary_unserialize($str);
var_dump($unserialized);
echo "\n";
var_export(serialize($data) === serialize($unserialized));
