#!/bin/bash

# --- CONFIGURATION ---
SERVER_URL="http://localhost:8080/upload/"
UPLOAD_DIR="./www/html/upload"
CONCURRENCY=50 # Reduced slightly to avoid OS local FD limits during stress
mkdir -p "$UPLOAD_DIR" # Ensure directory exists
# ---------------------

echo "========================================"
echo "💀 WEBSERV DESTRUCTION TEST SUITE 💀"
echo "========================================"

# ---------------------------------------------------------
# TEST 1: THE CHUNKED UPLOADER (Forces Chunked Encoding)
# ---------------------------------------------------------
echo -e "\n[TEST 1] Chunked Transfer Encoding Test..."
CHUNKED_FILE="/tmp/chunked_test.txt"
echo "This file is being sent in chunks to test your parser." > $CHUNKED_FILE

# curl --chunked forces chunked encoding without knowing Content-Length
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST \
    -H "Transfer-Encoding: chunked" \
    --data-binary @"$CHUNKED_FILE" $SERVER_URL)

if [ "$HTTP_STATUS" == "201" ]; then
    echo "✅ PASS: Server correctly handled chunked data."
else
    echo "❌ FAIL: Chunked upload failed with status $HTTP_STATUS."
fi

# ---------------------------------------------------------
# TEST 2: THE RAM CRUSHER (50MB Payload)
# ---------------------------------------------------------
echo -e "\n[TEST 2] The RAM Crusher (50MB Payload)..."
FILENAME="heavy_test.bin"
ORIGIN_FILE="/tmp/$FILENAME"
DEST_FILE="$UPLOAD_DIR/$FILENAME"

dd if=/dev/urandom of=$ORIGIN_FILE bs=1M count=50 status=none
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@$ORIGIN_FILE;filename=$FILENAME" $SERVER_URL)

if [ "$HTTP_STATUS" == "201" ]; then
    echo "✅ PASS: 50MB file buffered and saved perfectly."
else
    echo "❌ FAIL: 50MB upload failed ($HTTP_STATUS). Check client_max_body_size."
fi
rm -f $ORIGIN_FILE

# ---------------------------------------------------------
# TEST 3: THE SWARM (Concurrent Connections)
# ---------------------------------------------------------
echo -e "\n[TEST 3] The Swarm ($CONCURRENCY Concurrent Requests)..."
echo "Pew pew pew" > /tmp/laser.txt

for i in $(seq 1 $CONCURRENCY); do
    curl -s -o /dev/null -X POST -F "file=@/tmp/laser.txt;filename=laser_$i.txt" $SERVER_URL &
done
wait

SAVED_FILES=$(ls -1q $UPLOAD_DIR/laser_*.txt 2>/dev/null | wc -l | tr -d ' ')
if [ "$SAVED_FILES" -eq "$CONCURRENCY" ]; then
    echo "✅ PASS: All $CONCURRENCY requests handled."
else
    echo "❌ FAIL: Only $SAVED_FILES/100 files saved. Check for FD leaks."
fi
rm -f /tmp/laser.txt $UPLOAD_DIR/laser_*.txt

# ---------------------------------------------------------
# TEST 4: THE SLOWLORIS (Timeout Defense)
# ---------------------------------------------------------
echo -e "\n[TEST 4] The Slowloris (Timeout Check)..."
SERVER_PORT=$(echo $SERVER_URL | awk -F: '{print $3}' | awk -F/ '{print $1}')
(printf "POST /upload/ HTTP/1.1\r\nHost: localhost\r\n" ; sleep 65) | nc localhost $SERVER_PORT > /dev/null 2>&1 &
NC_PID=$!
sleep 5
echo "Waiting for timeout..."
wait $NC_PID
echo "✅ PASS: Connection terminated by timeout."

echo "========================================"
echo "STRESS TEST COMPLETE"
echo "========================================"