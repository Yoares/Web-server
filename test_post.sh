#!/bin/bash

# --- CONFIGURATION (CHANGE THESE TO MATCH YOUR SERVER) ---
SERVER_URL="http://localhost:8080/upload"  # The route handling your POST
UPLOAD_DIR="./www/html/upload"          # Where your server saves the file
# ---------------------------------------------------------

FILENAME="stress_test.bin"
ORIGIN_FILE="/tmp/$FILENAME"
DEST_FILE="$UPLOAD_DIR/$FILENAME"

echo "========================================"
echo "🚀 STARTING MULTIPART POST TEST"
echo "========================================"

# 1. Generate a 5MB raw binary file using /dev/urandom
# (Binary files are the hardest to parse because they contain random \r and \n characters)
echo "[1/5] Generating 5MB random binary file..."
dd if=/dev/urandom of=$ORIGIN_FILE bs=1M count=5 status=none

# 2. Calculate the original hash
ORIGIN_HASH=$(md5 -q $ORIGIN_FILE 2>/dev/null || md5sum $ORIGIN_FILE | awk '{print $1}')
echo "      Original MD5: $ORIGIN_HASH"

# 3. Send the file using curl's multipart flag (-F)
echo "[2/5] Uploading via cURL..."
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@$ORIGIN_FILE" $SERVER_URL)

echo "[3/5] HTTP Response Code: $HTTP_STATUS"
if [ "$HTTP_STATUS" != "201" ]; then
    echo "❌ ERROR: Expected 201 Created, got $HTTP_STATUS."
    rm -f $ORIGIN_FILE
    exit 1
fi

# 4. Check if the file arrived
echo "[4/5] Verifying uploaded file on server..."
if [ ! -f "$DEST_FILE" ]; then
    echo "❌ ERROR: File was not found at $DEST_FILE."
    rm -f $ORIGIN_FILE
    exit 1
fi

# 5. Calculate the uploaded file's hash
DEST_HASH=$(md5 -q $DEST_FILE 2>/dev/null || md5sum $DEST_FILE | awk '{print $1}')
echo "      Uploaded MD5: $DEST_HASH"

echo "========================================"
echo "[5/5] FINAL VERDICT:"

# 6. Compare hashes
if [ "$ORIGIN_HASH" == "$DEST_HASH" ]; then
    echo "✅ SUCCESS! Perfect byte-for-byte copy."
    echo "Your sliding-window boundary parser is bulletproof."
else
    echo "❌ FAILED! Hash mismatch. The file is corrupted."
    echo "Tip: Check if you are accidentally writing the '\r\n' that comes before the final boundary."
fi
echo "========================================"

# Cleanup
rm -f $ORIGIN_FILE
# rm -f $DEST_FILE # Uncomment if you want the script to delete the uploaded file automatically