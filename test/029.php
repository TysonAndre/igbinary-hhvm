<?php
// Igbinary module info
ob_start();
phpinfo(INFO_MODULES);
$str = ob_get_clean();

$array = explode("\n", $str);
$array = preg_grep('/^igbinary/', $array);

echo implode("\n", $array);
