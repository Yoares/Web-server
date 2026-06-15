#!/bin/bash

# ==========================================
# Configuration: Adjust these to your Webserv
# ==========================================
HOST="127.0.0.1"
PORT="8080"
# Point this to a valid PHP script in your server
URI="/cgi-bin/test.php" 

URL="http://${HOST}:${PORT}${URI}"
CONCURRENCY=50      # Number of simultaneous requests per batch
TOTAL_REQUESTS=500  # Total number of requests to send

# ==========================================

echo "🚀 Starting CGI Stress Test on $URL"
echo "Concurrency: $CONCURRENCY | Total Requests: $TOTAL_REQUESTS"
echo "------------------------------------------------------------"

# Create a temporary file to store the HTTP status codes
TMP_RESULTS=$(mktemp)

# Function to execute a single request
fire_request() {
    # -s: Silent mode (hides progress bar)
    # -o /dev/null: Discards the response body (we only care about headers/status)
    # -w: Extracts only the HTTP status code and prints it
    curl -s -o /dev/null -w "%{http_code}\n" "$URL" >> "$TMP_RESULTS"
}

# Track start time for basic benchmarking
START_TIME=$(date +%s)

# The Main Loop
for ((i=0; i<$TOTAL_REQUESTS; i+=$CONCURRENCY)); do
    
    # Spawn a batch of concurrent requests
    for ((j=0; j<$CONCURRENCY && (i+j)<$TOTAL_REQUESTS; j++)); do
        fire_request &  # The '&' puts the process in the background
    done
    
    # Wait for all background processes in the current batch to finish
    wait 
    
    # Visual progress indicator
    echo -n "█"
done

echo "" # Newline after progress bar
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo "------------------------------------------------------------"
echo "📊 Results Overview"
echo "Time taken: ${DURATION} seconds"
echo "Total Requests Logged: $(wc -l < "$TMP_RESULTS")"
echo ""
echo "HTTP Status Code Breakdown:"
# Sort the results, group them, and count occurrences
sort "$TMP_RESULTS" | uniq -c | sort -rn | while read count code; do
    if [ "$code" = "200" ]; then
        echo "  ✅ $code OK : $count requests"
    elif [ "$code" = "000" ]; then
        echo "  ❌ Failed to connect / Timeout : $count requests"
    else
        echo "  ⚠️  $code : $count requests"
    fi
done
echo "------------------------------------------------------------"

# Cleanup
rm -f "$TMP_RESULTS"