<?php
$_SERVER['DOCUMENT_ROOT']=getcwd();
$_SERVER['REQUEST_URI']="file://".getcwd()."t.php";
$_SERVER['REMOTE_ADDR']="127.0.0.1";
$_SERVER['SCRIPT_NAME']="/t.php";
include "./gridsite.inc";
?>
<html>
<p>Hello GridWorld!</p>
</html>
