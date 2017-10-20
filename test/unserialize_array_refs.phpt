<?php
// Unserialize array refs should be backwards compatible with v1.

error_reporting(E_ALL|E_STRICT);
$unserialized = igbinary_unserialize(base64_decode('AAAAARQCBgAUAwYAEQFhBgERAWIGAhEBYwYBAQE='));

printf("Type = %s\n", gettype($unserialized));
printf("Count = %d\n", count($unserialized));
printf("Value = %s\n", json_encode($unserialized));
printf("Same = %s\n", json_encode($unserialized[0] === $unserialized[1]));
