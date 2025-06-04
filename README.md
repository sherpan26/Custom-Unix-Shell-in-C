# Custom Unix Shell in C

This project implements a custom shell in C that mimics basic UNIX shell functionality such as executing commands, piping, redirection, and handling built-in commands like `cd`, `pwd`, and `exit`.

## 📁 Project Structure

- `mysh.c`: Main shell program supporting interactive and batch modes.
- `commands.c` / `commands.h`: Logic for built-in command handling and path resolution.
- `pipe.c`: Demonstrates inter-process communication using pipes.
- `test.c`: Standalone implementation of a mini-shell for testing input parsing and piping.
- `helloworld.c`: Sample test program.
- `Makefile` (inside `P3.tar`): Used to compile the project.

## ✨ Features

### 🧩 Built-in Commands
- `cd [dir]`: Change working directory
- `pwd`: Print current working directory
- `exit`: Exit the shell

### 🧵 External Command Execution
- Executes any command available in system paths (`/bin`, `/usr/bin`, etc.)
- Full support for absolute and relative paths

### 🔁 Piping
- Supports simple two-command pipes:
