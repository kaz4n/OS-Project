#!/bin/bash

echo "=== Phase 3 Multi-Client Concurrent Test Suite ==="
sleep 1

# Test 1: Verify persistent connection with multiple commands
echo -e "\n========== TEST 1: Persistent Connection =========="
echo "Sending 3 commands on same connection..."
{
  echo "echo 'Command 1 - First'"
  sleep 0.2
  echo "echo 'Command 2 - Second'"
  sleep 0.2
  echo "echo 'Command 3 - Third'"
  sleep 0.2
  echo "exit"
} | ./client 2>&1 | head -20

sleep 1

# Test 2: Concurrent clients (background processes)
echo -e "\n========== TEST 2: Concurrent Clients =========="
echo "Starting 3 concurrent clients in background..."

# Client A
{
  echo "echo 'ClientA-Command1'"
  sleep 0.3
  echo "echo 'ClientA-Command2'"
  sleep 0.3
  echo "exit"
} | ./client 2>&1 > /tmp/client_a.log &
PID_A=$!

# Client B
sleep 0.1
{
  echo "echo 'ClientB-Command1'"
  sleep 0.3
  echo "echo 'ClientB-Command2'"
  sleep 0.3
  echo "exit"
} | ./client 2>&1 > /tmp/client_b.log &
PID_B=$!

# Client C
sleep 0.1
{
  echo "echo 'ClientC-Command1'"
  sleep 0.3
  echo "echo 'ClientC-Command2'"
  sleep 0.3
  echo "exit"
} | ./client 2>&1 > /tmp/client_c.log &
PID_C=$!

# Wait for all clients to complete
wait $PID_A $PID_B $PID_C

echo "Client A output:"
cat /tmp/client_a.log | grep -E "(Connected|scheduler|Command)" | head -10
echo ""
echo "Client B output:"
cat /tmp/client_b.log | grep -E "(Connected|scheduler|Command)" | head -10
echo ""
echo "Client C output:"
cat /tmp/client_c.log | grep -E "(Connected|scheduler|Command)" | head -10

sleep 1

# Test 3: Verify scheduler metadata
echo -e "\n========== TEST 3: Scheduler Metadata Verification =========="
echo "Checking scheduler metadata format..."
{
  echo "whoami"
  sleep 0.2
  echo "pwd"
  sleep 0.2
  echo "exit"
} | ./client 2>&1 | grep scheduler

sleep 1

# Test 4: Pipeline test
echo -e "\n========== TEST 4: Pipeline Support =========="
echo "Testing pipe commands..."
{
  echo "echo 'line1' > /tmp/test_pipe.txt && echo 'line2' >> /tmp/test_pipe.txt"
  sleep 0.2
  echo "cat /tmp/test_pipe.txt | wc -l"
  sleep 0.2
  echo "exit"
} | ./client 2>&1 | grep -E "(scheduler|line|^[0-9])"

echo -e "\n========== All Tests Complete =========="
