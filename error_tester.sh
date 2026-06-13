#!/bin/bash

# Configuration
HOST="127.0.0.1"
PORT="8080"
TARGET="$HOST:$PORT"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}====================================================${NC}"
echo -e "${BLUE}  Webserv Error Handling Stress Tester ${NC}"
echo -e "${BLUE}====================================================${NC}\n"

# Helper function to send raw HTTP requests using netcat
send_raw_request() {
    local test_name=$1
    local request=$2
    local expected_code=$3

    echo -e "${YELLOW}[TEST] $test_name ${NC}"
    echo -e "Expecting: ${GREEN}$expected_code${NC}"
    
    # Send the request and grab the first line of the response (HTTP/1.1 XXX Status)
    response=$(printf "$request" | nc -w 1 $HOST $PORT | head -n 1)
    
    if [[ $response == *"$expected_code"* ]]; then
        echo -e "Result: ${GREEN}PASS${NC} -> $response"
    else
        echo -e "Result: ${RED}FAIL${NC} -> Got: $response"
    fi
    echo "----------------------------------------------------"
    sleep 0.1
}

# -------------------------------------------------------------------------
# TEST SUITE
# -------------------------------------------------------------------------

# 1. 404 Not Found
send_raw_request "1. Requesting non-existent file (404)" \
"GET /this_file_absolutely_does_not_exist_12345.html HTTP/1.1\r\nHost: $TARGET\r\n\r\n" \
"404"

# 2. 400 Bad Request (Missing Host Header)
# HTTP/1.1 strictly requires a Host header.
send_raw_request "2. Missing Host Header (400)" \
"GET / HTTP/1.1\r\n\r\n" \
"400"

# 3. 505 HTTP Version Not Supported
# Sending HTTP/2.0 or HTTP/1.0
send_raw_request "3. Unsupported HTTP Version (505)" \
"GET / HTTP/2.0\r\nHost: $TARGET\r\n\r\n" \
"505"

# 4. 501 Not Implemented
# Sending an unknown method that isn't GET, POST, DELETE, or HEAD
send_raw_request "4. Unknown HTTP Method (501)" \
"MAGIC / HTTP/1.1\r\nHost: $TARGET\r\n\r\n" \
"501"

# 5. 411 Length Required
# Sending a POST request without Content-Length or Transfer-Encoding: chunked
send_raw_request "5. POST without Content-Length (411)" \
"POST /upload HTTP/1.1\r\nHost: $TARGET\r\n\r\nbodydata" \
"411"

# 6. 400 Bad Request (Malformed Chunked Request)
# Sending invalid hex values in a chunked body
send_raw_request "6. Malformed Chunked Hex Size (400)" \
"POST /upload HTTP/1.1\r\nHost: $TARGET\r\nTransfer-Encoding: chunked\r\n\r\nNOT_A_HEX_SIZE\r\nData\r\n0\r\n\r\n" \
"400"

# 7. 431 Request Header Fields Too Large
# Generating a massive header field (> 8192 bytes limit set in your HttpRequest.cpp)
MASSIVE_HEADER=$(head -c 9000 < /dev/zero | tr '\0' 'A')
send_raw_request "7. Header Too Large (431)" \
"GET / HTTP/1.1\r\nHost: $TARGET\r\nX-Custom-Header: $MASSIVE_HEADER\r\n\r\n" \
"431"

# 8. 400 Bad Request (Space in Request URI)
# Request lines must be exactly "METHOD URI VERSION", spaces inside the URI are illegal
send_raw_request "8. Illegal Space in URI (400)" \
"GET /index html HTTP/1.1\r\nHost: $TARGET\r\n\r\n" \
"400"

# 9. 400 Bad Request (Empty Header Name)
# Sending a colon without a key before it
send_raw_request "9. Empty Header Name (400)" \
"GET / HTTP/1.1\r\nHost: $TARGET\r\n: empty_key_value\r\n\r\n" \
"400"

# 10. 413 Payload Too Large (Requires client_max_body_size config)
# Note: You may need to adjust the route to match your location block
send_raw_request "10. Payload Too Large (413) - Assuming low limit on route" \
"POST / HTTP/1.1\r\nHost: $TARGET\r\nContent-Length: 999999999\r\n\r\n" \
"413"

echo -e "${BLUE}Stress Test Complete!${NC}"