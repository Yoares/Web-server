<?php
echo "Content-Type: text/plain\r\n\r\n";

echo "--- PARSED POST DATA ---\n";
print_r($_POST);

echo "\n--- RAW BODY (php://input) ---\n";
echo file_get_contents("php://input");
echo "\n";
?>