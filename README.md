# My Local Shell (OS Project)

A simple custom shell written in C.

This project supports:
- Interactive shell loop with prompt
- Command parsing with quotes and escapes
- Pipeline execution (up to 4 commands)
- Built-in commands for common tasks
- External command execution via `execvp`

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
