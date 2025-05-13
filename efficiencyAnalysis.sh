#!/bin/bash
# Script to test server efficiency.
# Usage: ./efficiencyAnalysis.sh

# Ensure the server is running
if ! pgrep -f "dserver" > /dev/null; then
    echo "Error: The server is not running. Please start the server manually before running this script."
    exit 1
fi

ITERATIONS=20
CONSULTS=10
SEARCH_KEYWORD="praia"

LOG_FILE="efficiency_results.log"
echo "Efficiency Analysis Results" > "$LOG_FILE"
echo "----------------------------------------" >> "$LOG_FILE"

echo "Iterations: $ITERATIONS" >> "$LOG_FILE"
echo "Files: $CONSULTS" >> "$LOG_FILE"

# Test 1: Consulting
START_TIME=$(date +%s.%N)
for ((i = i; i <= ITERATIONS; i++)); do
    for ((j = 500; j <= CONSULTS + 500; j++)); do
        ./bin/dclient -c $j > /dev/null 2>&1
    done
done
END_TIME=$(date +%s.%N)

# Calculate elapsed time for Test 1
TEST1_TIME=$(echo "$END_TIME - $START_TIME" | bc)
echo "Total Consulting Time: $TEST1_TIME seconds" >> "$LOG_FILE"
echo "----------------------------------------" >> "$LOG_FILE"

# Test 2: Searching with different processes
# Search with default processes
START_TIME=$(date +%s.%N)
./bin/dclient -s "$SEARCH_KEYWORD" > /dev/null 2>&1
END_TIME=$(date +%s.%N)
TEST2_TIME=$(echo "$END_TIME - $START_TIME" | bc)
echo "Total Time for '/bin/dclient -s $SEARCH_KEYWORD': $TEST2_TIME seconds" >> "$LOG_FILE"

# Search with 10 processes
START_TIME=$(date +%s.%N)
./bin/dclient -s "$SEARCH_KEYWORD" 10 > /dev/null 2>&1
END_TIME=$(date +%s.%N)
TEST2_TIME=$(echo "$END_TIME - $START_TIME" | bc)
echo "Total Time for '/bin/dclient -s $SEARCH_KEYWORD 10': $TEST2_TIME seconds" >> "$LOG_FILE"

# Search with 100 processes
START_TIME=$(date +%s.%N)
./bin/dclient -s "$SEARCH_KEYWORD" 100 > /dev/null 2>&1
END_TIME=$(date +%s.%N)
TEST2_TIME=$(echo "$END_TIME - $START_TIME" | bc)
echo "Total Time for '/bin/dclient -s $SEARCH_KEYWORD 100': $TEST2_TIME seconds" >> "$LOG_FILE"
