<?php
// Check for serialization handler
$output = '';

function open($path, $name) {
	return true;
}

function close() {
	return true;
}

function read($id) {
	global $output;
	return pack('H*', '0000000214011103666f6f0601');
}

function write($id, $data) {
	global $output;
	$output .= substr(bin2hex($data), 8). "\n";
	return true;
}

function destroy($id) {
	return true;
}

function gc($time) {
	return true;
}

ini_set('session.serialize_handler', 'igbinary');

session_set_save_handler('open', 'close', 'read', 'write', 'destroy', 'gc');

session_start();

echo ++$_SESSION['foo'], "\n";

session_write_close();

echo $output;