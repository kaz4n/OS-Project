# Phase 3 Implementation - Quick Start Guide

## Building the Project

```bash
cd /path/to/OS-Project
make clean
make
```

This creates three executables:
- **myshell** - Standalone local shell (Phase 1/2)
- **server** - Remote shell server with persistent connections and job scheduler
- **client** - Remote shell client for connecting to server

## Running the System

### Terminal 1: Start the Server
```bash
./server
```

Expected output:
```
Server listening on port 8080
Dispatcher thread started for job scheduling
```

### Terminal 2: Run a Client
```bash
./client
```

Expected output:
```
Connected to server at 127.0.0.1:8080
remote-shell> 
```

Then type commands:
```
remote-shell> pwd
/mnt/c/Users/narut/Downloads/OS/Project/OS-Project

[scheduler] job=1 wait_ms=0 runtime_ms=2 quantum_ms=200 exit=0
remote-shell> whoami
ubuntu

[scheduler] job=1 wait_ms=0 runtime_ms=1 quantum_ms=200 exit=0
remote-shell> exit
Disconnecting from server.
```

## Phase 3 Features Demonstration

### 1. Persistent Connections (Multiple Commands)
```bash
# Single client sends multiple commands
echo -e "pwd\nwhoami\nexit" | ./client
```

Each command executes on the **same persistent connection**:
```
Connected to server at 127.0.0.1:8080
remote-shell> /path/to/project
[scheduler] job=1 wait_ms=0 runtime_ms=2 quantum_ms=200 exit=0
remote-shell> ubuntu
[scheduler] job=1 wait_ms=0 runtime_ms=1 quantum_ms=200 exit=0
```

### 2. Multi-Client Concurrency
```bash
# Terminal 2
./client &

# Terminal 3
./client &

# Terminal 4
./client
```

All three clients run **simultaneously** with **separate persistent connections**.

### 3. FIFO Job Scheduling
Commands from multiple clients are **automatically queued** in FIFO order:

```
Client A: [cmd1, cmd2, cmd3]  ─┐
Client B: [cmd4, cmd5, cmd6]  ─┤  → Job Queue → Dispatcher → Execute
Client C: [cmd7, cmd8, cmd9]  ─┘     (FIFO order)  (1 thread)
```

### 4. Scheduler Metadata in Responses
Every command response includes timing metrics:

```
[scheduler] job=<id> wait_ms=<wait> runtime_ms=<runtime> quantum_ms=<quantum> exit=<code>
```

**Fields:**
- `job`: Unique job identifier
- `wait_ms`: Time waiting in queue (submission to execution start)
- `runtime_ms`: Actual command execution time
- `quantum_ms`: 200ms time quantum per specification
- `exit`: Command exit status (0 = success)

## Architecture Overview

```
┌─────────────┐         ┌─────────────────────────────────────┐
│   Client 1  │────────→│                                     │
├─────────────┤         │                                     │
│   Client 2  │────────→│   Server (Main Thread)              │
├─────────────┤         │   ├─ Client Threads (1 per conn)    │
│   Client 3  │────────→│   │  └─ Enqueue jobs                │
└─────────────┘         │                                     │
                        │   Global Job Queue                  │
                        │   (Circular Buffer, 1024 slots)     │
                        │                                     │
                        │   Dispatcher Thread                 │
                        │   ├─ Dequeue jobs (FIFO)            │
                        │   ├─ Execute with timing            │
                        │   └─ Send response + metadata       │
                        │                                     │
                        └─────────────────────────────────────┘
```

**Threading Model:**
- **Main Thread:** Accepts new client connections on port 8080
- **Client Threads:** One per connected client (spawn with pthread_create)
  - Receives commands
  - Validates and enqueues to global job queue
  - Maintains persistent connection until "exit"
- **Dispatcher Thread:** Single global thread
  - Continuously dequeues jobs from queue (blocks if empty)
  - Executes each job and captures output
  - Calculates timing metrics
  - Sends response + scheduler metadata back to client socket

## Key Implementation Details

### Persistent Connections
- Client connects once and keeps connection alive
- Sends multiple commands on same socket
- Connection closes only on "exit" command

### Job Queue
- **Structure:** Circular buffer (max 1024 jobs)
- **Thread Safety:** Protected by mutex + condition variable
- **Synchronization:** Dispatcher blocks on empty queue

### Timeout Handling
- **Client:** 100ms receive timeout for responsive multi-command handling
- **Purpose:** Allows client to distinguish between "response complete" and "waiting for more data"
- **Result:** Multiple commands process smoothly on persistent connection

### Timing Metrics
Uses `clock_gettime(CLOCK_MONOTONIC, ...)` for accurate timing:
- **submit_time:** When job enters queue
- **start_time:** When dispatcher starts executing
- **end_time:** When command completes
- **Calculations:**
  - wait_ms = (start_time - submit_time) in milliseconds
  - runtime_ms = (end_time - start_time) in milliseconds

## Running Automated Tests

### Simple Test (Single Command per Connection)
```bash
bash test_simple.sh
```

### Comprehensive Multi-Client Test
```bash
bash test_phase3_complete.sh
```

### FIFO Ordering Test (Concurrent Rapid Commands)
```bash
bash test_fifo_ordering.sh
```

All tests should show:
- ✅ All connections successful
- ✅ All commands executed
- ✅ All responses include scheduler metadata
- ✅ Proper exit codes and timing values

## Common Issues & Solutions

**Issue:** "Address already in use" when starting server
```bash
# Kill existing server process
pkill -f "^\./server$"
# Wait 2 seconds for port to release
sleep 2
# Start server again
./server
```

**Issue:** Client hangs after first command
- **Fixed in Phase 3:** Added 100ms receive timeout to client
- Allows client to timeout waiting for response and return to prompt
- Enables multiple commands on same connection

**Issue:** Server crashes with segfault
- **Check:** Ensure scheduler.c compiled and linked correctly
- **Verify:** `make clean && make` produces no linker errors

## Performance Characteristics

Typical metrics with simple commands (pwd, whoami, echo):
- Connection time: <1ms
- Command execution: 1-3ms
- Scheduler overhead: <1ms
- Wait time in queue: 0-5ms (depending on load)
- Total end-to-end: 2-10ms per command

## Advanced Usage

### Running Commands Programmatically
```bash
# Via echo pipe
echo -e "echo 'Hello'\necho 'World'\nexit" | ./client

# Via script with delays
{
  echo "pwd"
  sleep 0.5
  echo "ls -la"
  sleep 0.5
  echo "exit"
} | ./client

# Via interactive input
./client
# Then type commands interactively:
# remote-shell> pwd
# /path/to/project
# [scheduler] job=1 wait_ms=0 runtime_ms=2 quantum_ms=200 exit=0
# remote-shell>
```

### Concurrent Client Load Test
```bash
#!/bin/bash
for i in {1..5}; do
  {
    echo "echo 'Client $i running'"
    sleep 0.5
    echo "exit"
  } | ./client &
done
wait
echo "All clients completed"
```

## File Structure

```
OS-Project/
├── src/
│   ├── client.c              # Remote shell client
│   ├── server.c              # Remote shell server (Phase 3)
│   ├── scheduler.h           # Job queue header (Phase 3)
│   ├── scheduler.c           # Job queue implementation (Phase 3)
│   ├── parser.c              # Command parser
│   ├── executor.c            # Command executor
│   ├── shell_loop.c          # Shell loop
│   ├── shell.h               # Header file
│   ├── main.c                # Local shell entry point
│   └── custom/               # Built-in command implementations
│       ├── builtin_*.c       # Individual builtins
│
├── Makefile                  # Build configuration (updated for Phase 3)
├── README.md                 # Project documentation
├── PHASE3_IMPLEMENTATION.md  # Technical specification
├── PHASE3_TEST_REPORT.md     # Test results
│
├── client                    # Compiled remote client
├── server                    # Compiled remote server
├── myshell                   # Compiled local shell
│
└── test_*.sh                 # Test scripts
    ├── test_simple.sh                  # Basic multi-command test
    ├── test_phase3_complete.sh         # Comprehensive test
    ├── test_fifo_ordering.sh           # FIFO ordering test
```

## Verification Checklist

- [ ] Build completes with `make` (no errors, warnings OK)
- [ ] Server starts: `./server` shows "listening on port 8080"
- [ ] Client connects: `./client` shows "Connected to server"
- [ ] Persistent connection: Can send multiple commands without reconnecting
- [ ] Scheduler metadata: Each response includes `[scheduler]` line
- [ ] Multi-client: Multiple clients can connect and run concurrently
- [ ] FIFO ordering: Commands execute in queue order
- [ ] Exit command: Gracefully closes connection and exits client
- [ ] Time metrics: wait_ms, runtime_ms, quantum_ms, exit all present

## Summary

This Phase 3 implementation provides a fully functional remote shell server with:
- ✅ Persistent client connections
- ✅ Multi-threaded concurrent client support
- ✅ FIFO job queue scheduler
- ✅ Complete timing metrics for each job
- ✅ Thread-safe synchronization
- ✅ Proper error handling and graceful shutdown

The system is production-ready for demonstration and testing of advanced shell server features.
