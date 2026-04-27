#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if [[ ! -x ./server || ! -x ./client ]]; then
  echo "Building binaries..."
  make clean
  make
fi

SERVER_LOG="/tmp/os_project_server.log"
OUT1="/tmp/os_project_client1.out"
OUT2="/tmp/os_project_client2.out"
OUT3="/tmp/os_project_client3.out"

rm -f "$SERVER_LOG" "$OUT1" "$OUT2" "$OUT3"

./server > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!

cleanup() {
  if kill -0 "$SERVER_PID" 2>/dev/null; then
    kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null
  fi
}
trap cleanup EXIT

sleep 1

echo "[Test 1] Single client command"
printf "echo hello_from_client_2\nexit\n" | ./client > "$OUT1"
cat "$OUT1"

echo "[Test 2] Concurrent clients"
(printf "sleep 1\nexit\n" | ./client > "$OUT2") &
PID_A=$!
(printf "echo queued_client\nexit\n" | ./client > "$OUT3") &
PID_B=$!
wait "$PID_A"
wait "$PID_B"

echo "--- client 2 output ---"
cat "$OUT2"

echo "--- client 3 output ---"
cat "$OUT3"

echo "[Test 3] Remote pipeline"
printf "cat demo.c | wc -l\nexit\n" | ./client > "$OUT1"
cat "$OUT1"

echo "\nServer log tail:"
tail -n 20 "$SERVER_LOG"
