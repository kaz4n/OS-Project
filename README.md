# My Local Shell (OS Project)

A simple custom shell written in C.

This project supports:
- Interactive shell loop with prompt
- Command parsing with quotes and escapes
- Pipeline execution (up to 4 commands)
- Built-in commands for common tasks
- External command execution via `execvp`
- Remote client/server shell over TCP sockets
- Multi-client request handling with threads
- Time-based dispatcher scheduler (job queue + timestamps + quantum)

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
cd "OS Project"
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

### Design

- Server uses one thread per client connection.
- Each client request is converted into a scheduler job.
- Jobs are queued in FIFO order with a submission timestamp.
- A dispatcher thread pops jobs from the queue and executes them.
- Command execution is isolated in a child process.
- Child `stdout` and `stderr` are redirected to a pipe, captured by the parent, and sent back only to the requesting client socket.
- Scheduler metadata is appended to each response:
	- job id
	- queue wait time in ms
	- runtime in ms
	- configured quantum in ms
	- command exit code

### Synchronization

- Scheduler queue access is protected with `pthread_mutex_t`.
- Dispatcher waits on `pthread_cond_t` when queue is empty.
- Each submitted job has its own condition variable for completion signaling back to the client thread.

### Time-Based Scheduling

- Scheduler quantum is currently set to `200 ms` by default.
- Dispatcher loop polls process completion using this quantum interval.
- Queue wait and execution duration are timestamped using `clock_gettime`.

## Reproducible Test Cases

Run in Linux/WSL for full process and pipe features.

Quick run (all Phase 3 tests):

```bash
bash ./phase3_tests.sh
```

### Build

```bash
make clean
make
```

### Test 1: Single Client Command + Scheduler Metadata

Terminal A:

```bash
./server
```

Terminal B:

```bash
printf "echo hello_from_client_2\nexit\n" | ./client
```

Expected:

- Normal command output is printed (current directory).
- Response ends with a scheduler line like:

```text
[scheduler] job=1 wait_ms=<number> runtime_ms=<number> quantum_ms=200 exit=0
```

Actual:

```text
hello_from_client_2

[scheduler] job=1 wait_ms=0 runtime_ms=201 quantum_ms=200 exit=0
```

### Test 2: Concurrent Clients (Queue + Threaded Handling)

Terminal A:

```bash
./server
```

Terminal B:

```bash
printf "sleep 1\nexit\n" | ./client
```

Terminal C (start immediately after B):

```bash
printf "echo queued_client\nexit\n" | ./client
```

Expected:

- Both clients get correct response for their own command.
- Each response includes a unique scheduler job id.
- Second client may show non-zero `wait_ms` when queued behind first.

Actual:

```text
Client-B (sleep 1)
[scheduler] job=2 wait_ms=0 runtime_ms=1201 quantum_ms=200 exit=0

Client-C (echo queued_client)
queued_client
[scheduler] job=3 wait_ms=1200 runtime_ms=201 quantum_ms=200 exit=0
```

### Test 3: Pipeline Command Through Remote Path

Terminal B:

```bash
printf "cat demo.c | wc -l\nexit\n" | ./client
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
