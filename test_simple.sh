#!/bin/bash

echo "=== Simple Multi-Client Functional Test ==="

# Test 1: Single command from one client
echo -e "\n[Test 1] Single command test"
echo "pwd" | timeout 3 ./client
sleep 1

# Test 2: Another single command
echo -e "\n[Test 2] Second single command test"
echo "whoami" | timeout 3 ./client
sleep 1

# Test 3: Multiple commands in sequence
echo -e "\n[Test 3] Multiple commands in one session"
{
  echo "echo 'First command'"
  sleep 0.3
  echo "echo 'Second command'"
  sleep 0.3
  echo "exit"
} | timeout 3 ./client
sleep 1

echo -e "\n=== Test completed ==="
