# MyShell Implementation

A robust shell implementation in C that provides comprehensive command-line functionality including pipes, redirections, and error handling.

## Features

### Basic Command Execution
- Interactive shell prompt (`$`)
- Execution of commands with and without arguments (`ls`, `ls -l`, etc.)
- Support for system commands through `execvp`
- Built-in commands (`cd`, `exit`)

### Input/Output Redirection
- Input redirection (`<`)
- Output redirection (`>`)
- Error redirection (`2>`)
- Append mode (`>>`)

### Pipeline Support
- Multiple command pipeline execution (`|`)
- Support for n-length pipelines
- Proper handling of pipe input/output

### Error Handling
- Missing file errors
- Command not found errors
- Syntax error detection
- Missing redirection target errors
- Pipeline errors
- Memory allocation errors

## Project Structure
```
.
├── Makefile
├── README.md
├── Report.pdf
└── src/
    ├── myshell.c    # Main shell loop and command processing
    ├── parser.c     # Command parsing and tokenization
    ├── parser.h     # Parser declarations
    ├── executor.c   # Command execution and pipeline handling
    └── executor.h   # Executor declarations
```

## Implementation Details

### Main Shell (myshell.c)
- Implements interactive command loop
- Handles built-in commands
- Manages command execution flow
- Provides error reporting

### Parser (parser.c)
- Tokenizes input commands
- Handles quoted strings
- Parses redirection operators
- Manages pipeline splitting
- Memory management for command structures

### Executor (executor.c)
- Manages process creation and execution
- Handles input/output redirection
- Implements pipeline execution
- Process synchronization and cleanup

## Building and Running

### Prerequisites
- GCC compiler
- UNIX-like environment (Linux/macOS)
- Make utility

### Building
```bash
make
```

### Cleaning (To clean uo compiled files)
```bash
make clean
```

### Running
```bash
./myshell
```

## Usage Examples

### Basic Commands
```bash
$ ls
$ ps aux
$ pwd
```

### Redirection
```bash
$ ls > output.txt //Output redirection
$ cat < input.txt //Input redirection
$ ls 2> error.log //Error redirection
$ echo "append" >> file.txt //Append mode
```

### Pipelines
```bash
$ ls | grep .txt
$ cat input.txt | sort | uniq
```

### Combined Operations
```bash
$ cat < input.txt | sort | uniq > output.txt
$ grep "error" < logs.txt | sort | uniq 2> error.log
```

## Error Handling Examples
- Missing input file: `cat < `
- Missing output file: `ls >`
- Invalid command: `invalidcmd`
- Pipeline errors: `ls | | grep test`
- File not found: `cat nonexistent.txt`

## Authors
- Rabeya Zahan Mily, rm6173
- Ziyue Tao, zt2221

## Course Information
CS-UH 3010 Operating Systems
Spring 2024
NYUAD
