#!/bin/bash

# --- CONFIGURATION ---
SERVER_URL="http://localhost:8080/upload/"
UPLOAD_DIR="./www/html/upload"
CONCURRENCY=100
# ---------------------

echo "========================================"
echo "💀 WEBSERV DESTRUCTION TEST SUITE 💀"
echo "========================================"

# ---------------------------------------------------------
# TEST 1: THE RAM CRUSHER (50MB Upload)
# ---------------------------------------------------------
echo -e "\n[TEST 1] The RAM Crusher (50MB Payload)..."
FILENAME="heavy_test.bin"
ORIGIN_FILE="/tmp/$FILENAME"
DEST_FILE="$UPLOAD_DIR/$FILENAME"

dd if=/dev/urandom of=$ORIGIN_FILE bs=1M count=50 status=none
ORIGIN_HASH=$(md5 -q $ORIGIN_FILE 2>/dev/null || md5sum $ORIGIN_FILE | awk '{print $1}')

HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@$ORIGIN_FILE;filename=$FILENAME" $SERVER_URL)
DEST_HASH=$(md5 -q $DEST_FILE 2>/dev/null || md5sum $DEST_FILE | awk '{print $1}')

if [ "$ORIGIN_HASH" == "$DEST_HASH" ] && [ "$HTTP_STATUS" == "201" ]; then
    echo "✅ PASS: 50MB file buffered and saved perfectly."
else
    echo "❌ FAIL: Hash mismatch or bad status code ($HTTP_STATUS)."
fi
rm -f $ORIGIN_FILE $DEST_FILE

# ---------------------------------------------------------
# TEST 2: THE SWARM (Concurrent Connections)
# ---------------------------------------------------------
echo -e "\n[TEST 2] The Swarm ($CONCURRENCY Concurrent Requests)..."
# We will send a small text file 100 times at the exact same moment
echo "Pew pew pew" > /tmp/laser.txt

# Fire off requests in the background
for i in $(seq 1 $CONCURRENCY); do
    curl -s -o /dev/null -X POST -F "file=@/tmp/laser.txt;filename=laser_$i.txt" $SERVER_URL &
done

# Wait for all background tasks to finish
wait

# Count how many files actually made it to the upload folder
SAVED_FILES=$(ls -1q $UPLOAD_DIR/laser_*.txt 2>/dev/null | wc -l)
# Clean up whitespace
SAVED_FILES=$(echo $SAVED_FILES | tr -d ' ')

if [ "$SAVED_FILES" -eq "$CONCURRENCY" ]; then
    echo "✅ PASS: All $CONCURRENCY requests handled successfully. Epoll is a beast."
else
    echo "❌ FAIL: Expected $CONCURRENCY files, but only found $SAVED_FILES. Sockets were dropped!"
fi
rm -f /tmp/laser.txt $UPLOAD_DIR/laser_*.txt

# ---------------------------------------------------------
# TEST 3: THE BAD CITIZEN (Malformed Headers)
# ---------------------------------------------------------
echo -e "\n[TEST 3] The Bad Citizen (Missing Host Header)..."
# HTTP/1.1 strictly requires a Host header.
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -H "Host:" $SERVER_URL)

if [ "$HTTP_STATUS" == "400" ]; then
    echo "✅ PASS: Server correctly rejected missing Host header with 400 Bad Request."
else
    echo "❌ FAIL: Expected 400, got $HTTP_STATUS."
fi

# ---------------------------------------------------------
# TEST 4: THE SLOWLORIS (Timeout Defense)
# ---------------------------------------------------------
echo -e "\n[TEST 4] The Slowloris (Timeout Check)..."
echo "Sending half a request and waiting..."

# We use netcat (nc) to open a TCP connection, send partial headers, and never close it.
# We then wait 65 seconds to see if your Webserv forcefully closes it (since your timeout is 60s).
SERVER_PORT=$(echo $SERVER_URL | awk -F: '{print $3}' | awk -F/ '{print $1}')

(
    printf "POST /upload/ HTTP/1.1\r\nHost: localhost\r\n"
    sleep 65
) | nc localhost $SERVER_PORT > /dev/null 2>&1 &
NC_PID=$!

sleep 5
echo "Connection opened. Webserv should be waiting... (Waiting 65 seconds for your checkTimeouts to fire)"
wait $NC_PID
echo "✅ PASS: Netcat connection was terminated. Check your server logs to ensure 'Connection timed out' was printed!"

echo "========================================"
echo "STRESS TEST COMPLETE"
echo "========================================"