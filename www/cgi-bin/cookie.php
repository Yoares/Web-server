
<?php
$session = isset($_COOKIE['session_id']) ? $_COOKIE['session_id'] : "No session found!";

// If there's no session, let's tell the browser to set one using the Set-Cookie header
if ($session === "No session found!") {
    $new_session = "PHP_SESS_" . rand(1000, 9999);
    echo "Set-Cookie: session_id=$new_session; Max-Age=3600\r\n";
    $session = $new_session . " (Just created!)";
}

echo "Content-Type: text/html\r\n\r\n";
echo "<h1>Cookie Tester</h1>";
echo "<p>Your current session data: <strong>$session</strong></p>";
?>