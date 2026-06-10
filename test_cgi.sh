#!/bin/bash

# ==========================================
# 42 WEBSERV CGI STRESS TESTER
# ==========================================

# Adjust these variables to match your config
HOST="127.0.0.1"
PORT="8080"
CGI_EXT="/directory/youpi.bla" # The path that triggers your ubuntu_cgi_tester
URL="http://$HOST:$PORT$CGI_EXT"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Starting 42 Webserv CGI Stress Test...${NC}\n"

# Helper function to check server pulse
check_server_alive() {
    if ! curl -s -m 2 $URL > /dev/null; then
        echo -e "${RED}[FATAL] Server crashed or is unresponsive!${NC}"
        exit 1
    fi
}

# ==========================================
# TEST 1: The "Non-Blocking" GET Test (50 Concurrent)
# ==========================================
echo -e "${GREEN}[TEST 1] Testing Non-Blocking Architecture (50 Concurrent GETs)...${NC}"
echo "If your server freezes here, your waitpid() or epoll setup is blocking."

for i in {1..50}; do
    curl -s -o /dev/null -w "%{http_code}" -X GET $URL &
done

wait
echo -e "\n${GREEN}Completed Test 1.${NC}\n"
check_server_alive

# ==========================================
# TEST 2: The "Memory Exhaustion" POST Test (100MB x 10 Concurrent)
# ==========================================
echo -e "${GREEN}[TEST 2] Testing Large Body Spooling (10 x 100MB Concurrent POSTs)...${NC}"
echo "Generating 100MB payload (this might take a second)..."
head -c 100000000 </dev/zero > /tmp/100mb_test.bin

echo "Firing requests... (If your server crashes, it's hoarding RAM or deadlocking pipes)"
for i in {1..10}; do
    curl -s -o /dev/null -w "%{http_code}" -X POST --data-binary @/tmp/100mb_test.bin $URL &
done

wait
rm -f /tmp/100mb_test.bin
echo -e "\n${GREEN}Completed Test 2.${NC}\n"
check_server_alive

# ==========================================
# TEST 3: The "Query String & Variables" Test
# ==========================================
echo -e "${GREEN}[TEST 3] Testing Query String and Environment Variables...${NC}"
echo "Sending GET request with complex query string..."

RES=$(curl -s -X GET "$URL?name=42student&project=webserv&score=125")
if echo "$RES" | grep -q "42student"; then
    echo -e "${GREEN}Pass: CGI successfully read the query string.${NC}"
else
    echo -e "${RED}Fail: CGI did not return the expected query string data.${NC}"
fi
echo -e "\n"

# ==========================================
# TEST 4: The "Chunked Transfer" Trick Test
# ==========================================
echo -e "${GREEN}[TEST 4] Testing Chunked Transfer Encoding on CGI POST...${NC}"
echo "Sending chunked POST request without Content-Length..."

RES=$(curl -s -X POST -H "Transfer-Encoding: chunked" -d "This is chunk 1" -d "This is chunk 2" $URL)
if echo "$RES" | grep -q "chunk"; then
    echo -e "${GREEN}Pass: CGI successfully received unchunked data.${NC}"
else
    echo -e "${YELLOW}Warning: Your server might not have passed the unchunked body to the CGI.${NC}"
fi
echo -e "\n"

# ==========================================
# TEST 5: The "Rapid Fire Connection Drop" (Orphan Zombies)
# ==========================================
echo -e "${GREEN}[TEST 5] Testing Zombie Process Cleanup (Rapid Fire & Drop)...${NC}"
echo "Spawning CGI processes and instantly closing the client connection..."

for i in {1..20}; do
    # -m 0.1 forces curl to timeout and drop the connection after 100ms, 
    # well before the CGI finishes executing!
    curl -s -m 0.1 -X GET $URL > /dev/null 2>&1 &
done
wait

echo -e "Check your server terminal. Did it crash? Type 'ps aux | grep ubuntu_cgi' to check for zombies."
echo -e "${GREEN}Completed Test 5.${NC}\n"
check_server_alive

echo -e "${YELLOW}All automated stress tests completed!${NC}"