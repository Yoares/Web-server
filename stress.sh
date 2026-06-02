#!/bin/bash

# ==========================================
# 💀 THE DOOMSDAY STRESS SCRIPT 💀
# Warning: This will aggressively attack your Webserv
# ==========================================

PORT=8080
BASE_URL="http://localhost:$PORT"
UPLOAD_URL="$BASE_URL/upload"
CONCURRENCY=200       # Pushing the limits of your FD table
RAPID_FIRE=1000       # Total requests for the barrage
HEAVY_SIZE=100        # Size in Megabytes for the massive upload

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${RED}================================================${NC}"
echo -e "${RED}🔥 INITIATING DOOMSDAY STRESS PROTOCOL 🔥${NC}"
echo -e "${RED}================================================${NC}"
mkdir -p ./www/html/upload

# ---------------------------------------------------------
# PHASE 1: THE MACHINE GUN (High Throughput / Keep-Alive)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 1] The Machine Gun ($RAPID_FIRE sequential GETs)...${NC}"
echo "Testing how fast your server can clear its event queue without leaking memory."

START_TIME=$(date +%s)
SUCCESS=0
for i in $(seq 1 $RAPID_FIRE); do
    # -s: silent, -w: write out HTTP status, -o: output to dev/null
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/")
    if [ "$STATUS" == "200" ]; then
        ((SUCCESS++))
    fi
done
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

if [ "$SUCCESS" -eq "$RAPID_FIRE" ]; then
    echo -e "${GREEN}✅ SURVIVED: $SUCCESS/$RAPID_FIRE requests handled in $DURATION seconds.${NC}"
else
    echo -e "${RED}❌ FAILED: Dropped $((RAPID_FIRE - SUCCESS)) requests. Socket bottleneck!${NC}"
fi

# ---------------------------------------------------------
# PHASE 2: THE FD EXHAUSTION (200 Concurrent Connections)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 2] The Swarm ($CONCURRENCY Concurrent POSTs)...${NC}"
echo "Attempting to exhaust your File Descriptors and cause EAGAIN errors."

echo "pew pew" > /tmp/laser.txt
for i in $(seq 1 $CONCURRENCY); do
    curl -s -o /dev/null -X POST -F "file=@/tmp/laser.txt;filename=swarm_$i.txt" "$UPLOAD_URL/" &
done
wait # Wait for all background processes to finish

SAVED=$(ls -1q ./www/html/upload/swarm_*.txt 2>/dev/null | wc -l | tr -d ' ')
if [ "$SAVED" -eq "$CONCURRENCY" ]; then
    echo -e "${GREEN}✅ SURVIVED: All $CONCURRENCY files saved. Epoll is bulletproof.${NC}"
else
    echo -e "${RED}❌ FAILED: Only $SAVED/$CONCURRENCY files saved. Server dropped connections!${NC}"
fi
rm -f ./www/html/upload/swarm_*.txt /tmp/laser.txt

# ---------------------------------------------------------
# PHASE 3: THE TITAN (Massive File Streaming)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 3] The Titan (${HEAVY_SIZE}MB Upload)...${NC}"
echo "Testing your server's ability to stream directly to disk without loading into RAM."

dd if=/dev/urandom of=/tmp/titan.bin bs=1M count=$HEAVY_SIZE status=none
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@/tmp/titan.bin;filename=titan.bin" "$UPLOAD_URL/")

if [[ "$STATUS" == "201" || "$STATUS" == "200" ]]; then
    echo -e "${GREEN}✅ SURVIVED: ${HEAVY_SIZE}MB file digested and saved perfectly.${NC}"
elif [ "$STATUS" == "413" ]; then
    echo -e "${YELLOW}⚠️ BLOCKED: Server returned 413. (Increase client_max_body_size to test full capacity).${NC}"
else
    echo -e "${RED}❌ FAILED: Upload crashed or returned status $STATUS.${NC}"
fi
rm -f /tmp/titan.bin ./www/html/upload/titan.bin

# ---------------------------------------------------------
# PHASE 4: THE HYDRA (Concurrent Slowloris Attack)
# ---------------------------------------------------------
echo -e "\n${CYAN}[PHASE 4] The Hydra (Concurrent Slowloris)...${NC}"
echo "Opening 20 dead connections and checking if the server can still answer legitimate requests."

# Spawn 20 background netcat processes that just hang there doing nothing
for i in $(seq 1 20); do
    (printf "GET / HTTP/1.1\r\nHost: localhost\r\n" ; sleep 15) | nc localhost $PORT > /dev/null 2>&1 &
done

sleep 2 # Let the slow connections establish

# While the server is busy hanging onto 20 dead sockets, fire a real request!
STATUS=$(curl --max-time 3 -s -o /dev/null -w "%{http_code}" "$BASE_URL/")

if [ "$STATUS" == "200" ]; then
    echo -e "${GREEN}✅ SURVIVED: Server remained responsive while under attack!${NC}"
else
    echo -e "${RED}❌ FAILED: Server choked and ignored legitimate traffic. Status: $STATUS${NC}"
fi

# Wait for the slow connections to naturally timeout/die
echo "Waiting for dead sockets to clear..."
wait

echo -e "\n${RED}================================================${NC}"
echo -e "${GREEN}STRESS PROTOCOL COMPLETE. CHECK YOUR SERVER LOGS.${NC}"
echo -e "${RED}================================================${NC}"