<?php
// Note: php-cgi might automatically generate the Content-Type header for you, 
// but it's good practice to output it if it doesn't.
echo "Content-Type: text/plain\r\n\r\n";

echo "--- GET VARIABLES ---\n";
print_r($_GET);

echo "\n--- CGI/SERVER VARIABLES ---\n";
// This will show exactly what your C++ server passed in the envp array
print_r($_SERVER);
?>