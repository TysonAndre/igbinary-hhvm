<?php
// Check for serialization handler, SessionHandlerInterface
// https://github.com/igbinary/igbinary/issues/23
// http://www.php.net/manual/en/class.sessionhandlerinterface.php
$output = '';

class S implements SessionHandlerInterface {
    public function open($path, $name) {
        return true;
    }

    public function close() {
        return true;
    }

    public function read($id) {
        global $output;
        $output .= "read\n";
        return pack('H*', '0000000214011103666f6f0601');
    }

    public function write($id, $data) {
        global $output;
        $output .= "wrote: ";
        $output .= substr(bin2hex($data), 8). "\n";
        return true;
    }

    public function destroy($id) {
        return true;
    }

    public function gc($time) {
        return true;
    }
}

class Foo {
}

class Bar {
}

ini_set('session.serialize_handler', 'igbinary');

$handler = new S();
session_set_save_handler($handler, true);

$db_object = new Foo();
$session_object = new Bar();

$v = session_start();
var_dump($v);
$_SESSION['test'] = "foobar";

session_write_close();

echo $output;