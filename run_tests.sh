#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

echo "Building project..."
make clean >/dev/null 2>&1 || true
make

printf '\n=== Phase 1: Local shell (myshell) quick check ===\n'
printf "pwd_new\nexit_new\n" | ./myshell

printf '\n=== Phase 2: Remote client/server quick check ===\n'
./server > /tmp/os_project_server.log 2>&1 &
SERVER_PID=$!
cleanup() {
	if kill -0 "$SERVER_PID" 2>/dev/null; then
		kill "$SERVER_PID" 2>/dev/null || true
		wait "$SERVER_PID" 2>/dev/null || true
	fi
}
trap cleanup EXIT

sleep 1
printf "echo_new hello_remote\nexit\n" | ./client

printf '\n=== Phase 3: Scheduler / concurrency check ===\n'

# start a long-running job
(printf "sleep 1\nexit\n" | ./client) &
PID_A=$!
# start a second short job queued behind first
(printf "echo queued_client\nexit\n" | ./client) &
PID_B=$!
wait "$PID_A"
wait "$PID_B"

printf '\nServer log (tail):\n'
tail -n 30 /tmp/os_project_server.log || true

printf '\nRun notes: For full interactive exploration, run ./myshell or start ./server and connect with ./client.\n'
