# Custom Unix Shell in C

A minimal UNIX-like shell written in C that supports running external commands, basic built-ins, and simple piping. Includes interactive mode and batch/script mode.

## Features
- **Interactive mode** (REPL-style prompt)
- **Batch mode** (run commands from a file)
- **Built-in commands**: `cd`, `pwd`, `exit`
- **External command execution** via `fork()` + `execvp()`
- **Simple piping** (two commands: `cmd1 | cmd2`)
- **Basic parsing/tokenization** of user input

## Project Structure
```text
.
├── mysh.c         # Main shell (interactive + batch)
├── commands.c     # Built-ins + helpers
├── commands.h
├── pipe.c         # Pipe demo / helper logic
├── test.c         # Testing / parsing experiments
├── helloworld.c   # Small sample program
└── makefile       # Build automation
```

## Build
```bash 
make
```
## Run
```bash
Interactive mode
./mysh
Batch mode (run commands from a file)
./mysh <script_file>
```
## Examples
```bash
./mysh
mysh> pwd
mysh> ls
mysh> ls | wc
mysh> exit
Notes / Limitations
Piping support is intended for one pipe (A | B) based on the current implementation.

Behavior may vary depending on how your parser handles quotes, multiple spaces, redirects, etc.
```

## Future Improvements
```bash
Support multiple pipes (A | B | C)

Add I/O redirection (>, <) in the main shell path

Add quoting + escaping

Better error messages + exit codes
```
