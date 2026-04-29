#!/bin/bash

echo "=== Multi-Client Test Suite ==="
sleep 1

# Test 1: Single client with multiple persistent commands
echo -e "\n[Test 1] Single client - persistent connection with 3 commands"
(
  sleep 0.5
  echo "pwd"
  sleep 0.2
  echo "whoami"
  sleep 0.2
  echo "exit"
) | ./client

sleep 1

# Test 2: Two concurrent clients
echo -e "\n[Test 2] Concurrent client 1"
(
  sleep 0.5
  echo "echo 'Client1 Command1'"
  sleep 0.3
  echo "echo 'Client1 Command2'"
  sleep 0.2
  echo "exit"
) | ./client &

echo -e "\n[Test 3] Concurrent client 2"
(
  sleep 0.5
  echo "echo 'Client2 Command1'"
  sleep 0.3
  echo "echo 'Client2 Command2'"
  sleep 0.2
  echo "exit"
) | ./client &

wait

echo -e "\n=== All tests completed ==="
