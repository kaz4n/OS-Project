# My Local Shell (OS Project)

A simple custom shell written in C supporting local execution, remote client/server architecture, and Phase 3 multitasking with time-based job scheduling.

## Phase 3: Remote Multitasking CLI Shell (Current Implementation)

This is the **Phase 3** implementation featuring:
- **Persistent Client Connections** - Single connection per client, multiple commands until "exit"
- **FIFO Job Scheduler** - Time-based job queue with fairness, timestamps, and execution metrics
- **Multi-threaded Dispatcher** - One dispatcher thread processes queued jobs while client threads handle socket I/O
- **Scheduler Metadata** - Every response includes: job_id, wait_ms, runtime_ms, quantum_ms, exit_code
- **Parallel Multi-Client Support** - Multiple concurrent persistent connections, each with independent job sequences

### Quick Start (Phase 3)

**Terminal 1 - Start Server:**
```bash
./server
# Output: "Server listening on port 8080"
#         "Dispatcher thread started for job scheduling"
```

**Terminal 2+ - Connect Clients (Persistent):**
```bash
./client
remote-shell> pwd_new
/path/to/project
[scheduler] job=1 wait_ms=0 runtime_ms=150 quantum_ms=200 exit=0

remote-shell> echo "Multiple commands on same connection!"
Multiple commands on same connection!
[scheduler] job=2 wait_ms=0 runtime_ms=100 quantum_ms=200 exit=0

remote-shell> exit
Disconnecting from server.
```

**Key Difference from Phase 2:** Connection stays open for multiple commands. No reconnection overhead!

---

## Project Features

This project supports:
- Interactive shell loop with prompt
- Command parsing with quotes and escapes
- Pipeline execution (up to 4 commands)
- Built-in commands for common tasks
- External command execution via `execvp`
- Remote client/server shell over TCP sockets
- Multi-client request handling with threads
- Time-based dispatcher scheduler (job queue + timestamps + quantum)
 - Remote client/server shell over TCP sockets
 - Multi-client request handling with threads

## Project Structure

- `src/` core shell source code
- `src/custom/` builtin command implementations
- `Makefile` build rules
- `myshell` compiled binary (after build)

## Requirements

### Linux / WSL (recommended)
- `gcc`
- `make`

On Ubuntu/WSL:

```bash
sudo apt update
sudo apt install build-essential
```

## Getting Started

### 1. Clone or copy the project

```bash
git clone <your-repo-url>
cd "OS-Project"
```

### 2. Build

```bash
make
```

This creates the executable `myshell`.

### 3. Run

```bash
./myshell
```

## Windows Note

Pipeline execution in this project is implemented for POSIX systems.

If you run natively on Windows, pipeline behavior will be limited and the program will suggest using WSL/Linux.

Use WSL for full features:

```powershell
wsl
cd "/mnt/c/Users/narut/Downloads/OS Project"
make
./myshell
```

## Built-in Commands

Your shell currently has 20 built-ins:

- `cd` / `cd_new`
- `pwd_new`
- `ls_new`
- `mkdir_new`
- `rm_new`
- `echo_new`
- `whoami_new`
- `clear_new`
- `exit_new`
- `help_new`
- `date_new`
- `uname_new`
- `touch_new`
- `cat_new`
- `head_new`
- `tail_new`
- `cp_new`
- `mv_new`
- `rmdir_new`
- `wc_new`

## Usage Examples

### Basic built-ins

```bash
pwd_new
ls_new
mkdir_new testdir
cd_new testdir
touch_new notes.txt
echo_new Hello Shell
cat_new notes.txt
whoami_new
date_new
uname_new
help_new
```

### File operations

```bash
cp_new demo.c demo_copy.c
mv_new demo_copy.c demo_moved.c
wc_new demo.c
head_new -n 5 demo.c
tail_new -n 5 demo.c
rm_new demo_moved.c
rmdir_new testdir
```

### External commands

```bash
ls -la
gcc --version
```

### Pipelines

```bash
cat demo.c | wc -l
ls | grep .c
cat demo.c | grep include | wc -l
```

## Phase 3 Report Summary

### Design (UPDATED - Scheduler Now Implemented)

- **Server Architecture:** One thread per client connection + one dedicated dispatcher thread
- **Job Queue:** FIFO scheduler with thread-safe queue protected by `pthread_mutex_t`
- **Job Tracking:** Each job has submission timestamp, start time, end time for accurate timing metrics
- **Job Processing:** Dispatcher thread continuously dequeues and executes jobs with timing measurement
- **Command Execution:** Isolated child process via fork; stdout/stderr redirected via pipes to client
- **Scheduler Metadata:** Every response includes job_id, wait_ms (submission→execution), runtime_ms (execution duration), quantum_ms (200ms), and exit_code
- **Persistent Connections:** Client maintains single TCP connection, sends multiple commands until "exit"

### Synchronization

- **Queue Access:** Protected with `pthread_mutex_t` in `scheduler.c`
- **Producer (Client Threads):** Lock → enqueue job → signal condvar → unlock
- **Consumer (Dispatcher Thread):** Lock → wait on condvar if empty → dequeue → unlock
- **Atomicity:** All critical sections locked; no race conditions
- **Fairness:** FIFO ordering ensures job fairness; timestamp tracking proves scheduling

### Parallel Execution Model

```
Main Thread
    ↓
Listens for clients → Creates client threads (1 per connection)
    
Each Client Thread:
    - Loops: recv command → enqueue job → (wait for execution)
    - Persistent: stays alive until "exit" received
    
Dispatcher Thread:
    - Continuously: dequeue job → fork → execute → capture output
    - Sends result back to client via socket
    - Returns to queue to get next job
```

### Key Phase 3 Improvements

| Feature | Phase 2 | Phase 3 |
|---------|---------|---------|
| Client Connection | New per command | Single persistent |
| Command Handling | Immediate execution | Queued & scheduled |
| Job Fairness | N/A | FIFO with timestamps |
| Timing Data | None | wait_ms, runtime_ms, job_id |
| Multi-command Sessions | Not supported | Full support |
| Concurrent Load | Limited | Efficient queuing |

## Phase 3 Test Cases

### Automated Test Suite

Run the comprehensive Phase 3 test suite:

```bash
chmod +x test_phase3.sh
bash ./test_phase3.sh
```

This executes:
- Single client with multiple persistent commands
- Multiple parallel clients (concurrent connections)
- Scheduler metadata verification
- Pipeline support with scheduler
- FIFO job queue fairness
- Compilation and build verification

### Manual Test 1: Persistent Connection (Single Client)

**Terminal 1:**
```bash
./server
```

**Terminal 2:**
```bash
./client
remote-shell> pwd_new
/mnt/c/Users/narut/Downloads/OS/Project/OS-Project
[scheduler] job=1 wait_ms=0 runtime_ms=150 quantum_ms=200 exit=0

remote-shell> ls
client  demo  demo.c  Makefile  myshell  server  src
[scheduler] job=2 wait_ms=0 runtime_ms=80 quantum_ms=200 exit=0

remote-shell> echo "Phase 3 working!"
Phase 3 working!
[scheduler] job=3 wait_ms=0 runtime_ms=100 quantum_ms=200 exit=0

remote-shell> exit
Disconnecting from server.
```

**Verification:** Same socket used for all 3 commands; no reconnection.

### Manual Test 2: Multiple Concurrent Clients

**Terminal 1:**
```bash
./server
```

**Terminal 2:**
```bash
./client
remote-shell> echo "Client A - Command 1"
Client A - Command 1
[scheduler] job=1 wait_ms=0 runtime_ms=100 quantum_ms=200 exit=0

remote-shell> (wait for Terminal 3 to send commands)
```

**Terminal 3 (Parallel):**
```bash
./client
remote-shell> echo "Client B - Command 1"
Client B - Command 1
[scheduler] job=2 wait_ms=15 runtime_ms=95 quantum_ms=200 exit=0
# Note: wait_ms=15 because it queued behind Client A

remote-shell> echo "Client B - Command 2"
Client B - Command 2
[scheduler] job=3 wait_ms=0 runtime_ms=85 quantum_ms=200 exit=0

remote-shell> exit
```

**Verification:** 
- Both clients maintain separate persistent connections
- Jobs execute in FIFO order (job 1, 2, 3)
- Job 2 has wait_ms > 0 (queued behind job 1)

### Manual Test 3: Pipelines with Scheduler

```bash
./client
remote-shell> cat src/shell.h | wc -l
50
[scheduler] job=1 wait_ms=0 runtime_ms=120 quantum_ms=200 exit=0

remote-shell> ls | grep .c | wc -l
8
[scheduler] job=2 wait_ms=0 runtime_ms=100 quantum_ms=200 exit=0

remote-shell> exit
```

**Verification:** Pipelines work correctly; scheduler metadata appended to output.

## Reproducible Test Cases (All Phases)

Run all automated checks for the project (Phase 1–3):

```bash
bash ./run_tests.sh
```

Or run just the Phase 3 tests:

```bash
bash ./test_phase3.sh
```

### Build

```bash
make clean
make
```

### Test 1: Phase 1 (Local shell)

```bash
make
printf "pwd_new\nexit_new\n" | ./myshell
```

Expected output: Current working directory path

### Test 2: Phase 3 (Remote multitasking with scheduler)

**Terminal 1:**
```bash
./server
```

**Terminal 2:**
```bash
./client
remote-shell> echo "Job 1"
Job 1
[scheduler] job=1 wait_ms=0 runtime_ms=100 quantum_ms=200 exit=0

remote-shell> pwd_new
/mnt/c/Users/narut/Downloads/OS/Project/OS-Project
[scheduler] job=2 wait_ms=0 runtime_ms=80 quantum_ms=200 exit=0

remote-shell> exit
```

**Verification:**
- Persistent connection (same socket for both commands)
- Scheduler metadata shows job sequence (job=1, job=2)
- No reconnection overhead

### Test 3: Pipelines

Pipelines are supported (POSIX only). Example (local):

```bash
printf "cat demo.c | wc -l\nexit\n" | ./myshell
```

Expected:

- Line-count result from pipeline.
- Scheduler footer appended.

Actual:

```text
2
[scheduler] job=4 wait_ms=0 runtime_ms=200 quantum_ms=200 exit=0
```

## Common Issues

### `make clean` fails in PowerShell
The Makefile uses `rm -f`, which is a Unix command.

Use WSL:

```powershell
wsl make clean
wsl make
```

### Command not found (`ws_new`)
Use `wc_new` (word count), not `ws_new`.

## Development Notes

- Max input line length: `1024`
- Max args per command: `64`
- Max commands in a pipeline: `4`

Values are defined in `src/shell.h`.
