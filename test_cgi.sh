#!/bin/bash

# ==========================================
# 💀 CGI DOOMSDAY STRESS SCRIPT 💀
# ==========================================

PORT=8080
BASE_URL="http://localhost:$PORT"
CGI_FAST="$BASE_URL/cgi-bin/fast.py"
CGI_SLEEP="$BASE_URL/cgi-bin/sleep.py"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${RED}================================================${NC}"
echo -e "${RED}🔥 INITIATING CGI STRESS PROTOCOL 🔥${NC}"
echo -e "${RED}================================================${NC}"

# ---------------------------------------------------------
# PHASE 1: THE FORK BOMB (150 Concurrent Execves)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 1] The Fork Bomb (150 Concurrent CGIs)...${NC}"
echo "Testing if your server leaks zombies or crashes from rapid fork() calls."

SUCCESS=0
for i in $(seq 1 150); do
    curl -s -o /dev/null -w "%{http_code}" -X GET "$CGI_FAST" &
done
wait

echo -e "${YELLOW}Check your terminal running webserv.${NC}"
echo -e "Run: ${GREEN}ps aux | grep Z${NC} in another terminal."
echo "If you have 'defunct' processes, you are leaking zombies!"

# ---------------------------------------------------------
# PHASE 2: THE PIPE STUFFER (Concurrent Large POST to CGI)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 2] The Pipe Stuffer (50 Concurrent 1MB POSTs)...${NC}"
echo "Testing if writing to CGI pipes blocks your main epoll loop."

dd if=/dev/urandom of=/tmp/cgi_payload.bin bs=1M count=1 status=none
for i in $(seq 1 50); do
    curl -s -o /dev/null -X POST --data-binary "@/tmp/cgi_payload.bin" "$CGI_FAST" &
done
wait

echo -e "${GREEN}✅ Phase 2 complete. If your server didn't freeze or return 500s, your pipes are non-blocking.${NC}"
rm -f /tmp/cgi_payload.bin

# ---------------------------------------------------------
# PHASE 3: THE TIME LORD (Hanging CGI Timeout)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 3] The Time Lord (Timeout Enforcement)...${NC}"
echo "Triggering a CGI script that sleeps for 10 seconds."
echo "Your server should kill it and return 504 Gateway Timeout after your configured limit (e.g., 5 seconds)."

START_TIME=$(date +%s)
# Send request to the sleeping CGI
STATUS=$(curl -m 12 -s -o /dev/null -w "%{http_code}" "$CGI_SLEEP")
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

if [ "$STATUS" == "504" ] || [ "$STATUS" == "500" ]; then
    echo -e "${GREEN}✅ SURVIVED: Server caught the hanging CGI and returned $STATUS in $DURATION seconds.${NC}"
elif [ "$STATUS" == "200" ]; then
    echo -e "${RED}❌ FAILED: Server waited the full 10 seconds. You are not enforcing CGI timeouts!${NC}"
else
    echo -e "${YELLOW}⚠️ Server returned $STATUS. Did the connection drop?${NC}"
fi

# ---------------------------------------------------------
# PHASE 4: THE MULTITASKER (CGI + Static Interleaving)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 4] The Multitasker...${NC}"
echo "Starting 10 slow CGIs in the background..."
for i in $(seq 1 10); do
    curl -s -o /dev/null "$CGI_SLEEP" &
done

echo "Attempting to fetch a normal webpage while CGIs are blocking..."
STATUS=$(curl --max-time 2 -s -o /dev/null -w "%{http_code}" "$BASE_URL/")

if [ "$STATUS" == "200" ]; then
    echo -e "${GREEN}✅ SURVIVED: Server served static files while CGIs were processing!${NC}"
else
    echo -e "${RED}❌ FAILED: Server froze! Your waitpid() or pipe reading is blocking the epoll loop.${NC}"
fi
wait

echo -e "\n${RED}================================================${NC}"
echo -e "${GREEN}CGI STRESS PROTOCOL COMPLETE.${NC}"
echo -e "${RED}================================================${NC}"