# SO-project
## Treasure Hunt System - System Programming Project

## Project Overview
This project involves building a **C program** in a UNIX environment to simulate a **treasure hunt game system**. The system allows users to create, manage, and participate in digital treasure hunts, utilizing file operations, processes, signals, pipes, and redirects for managing game state and inter-component communication.

The project is divided into **three phases**, each building on the previous one. Each phase must be implemented, committed to the Git repository, and presented during the lab sessions. **Failure to implement or submit any phase will result in disqualification of the entire project.**

---

## Phase 1: File Systems

### Goal
Create the foundation for storing and managing treasure hunt data using file operations.

### Requirements
Develop a program called **`treasure_manager`** that:
- Creates and reads treasure data files in a structured binary format.
- Stores treasure information with the following fields:
  - **Treasure ID** (unique identifier)
  - **User name** (unique, text)
  - **GPS coordinates** (latitude and longitude as floating-point numbers)
  - **Clue text** (string)
  - **Value** (integer)
- Supports the following command-line operations:
  - `add <hunt_id>`: Add a new treasure to the specified hunt.
  - `list <hunt_id>`: List all treasures in a hunt, including hunt name, total file size, and last modification time.
  - `view <hunt_id> <id>`: View details of a specific treasure.
  - `remove_treasure <hunt_id> <id>`: Remove a specific treasure.
  - `remove_hunt <hunt_id>`: Remove an entire hunt.
- Stores treasures in a file (or multiple files) within a hunt-specific directory.
- Logs all user operations in a `logged_hunt` text file in the hunt directory.
- Creates a symbolic link (`logged_hunt-<ID>`) in the program’s root directory for each hunt’s `logged_hunt` file.

### System Calls
- File operations: `open()`, `close()`, `read()`, `write()`, `lseek()`
- File information: `stat()` or `lstat()`
- Directory creation: `mkdir()`

### Notes
- Treasures are stored in fixed-size records in a binary format.
- Multiple treasures are stored in a single file, with optional grouping (e.g., by user).
- Proper error handling and data validation are required.
- Example command: `treasure_manager --remove 'game7' 'treasure2'`

### Deliverables
- A working `treasure_manager` program that manages records, hunts, logs operations, and creates symlinks.

### Hints
- Each hunt is stored in a separate directory.
- Hunt IDs can be the directory name or a unique identifier (e.g., `Hunt001`).
- No command requires listing all hunts, so directory parsing is not mandatory.

---

## Phase 2: Processes & Signals

### Goal
Extend the system to use multiple processes and signal-based communication.

### Requirements
Develop a new program called **`treasure_hub`** that provides an interactive interface with the following commands:
- `start_monitor`: Starts a background process to monitor hunts.
- `list_hunts`: Requests the monitor to list all hunts and their total treasure count.
- `list_treasures`: Displays all treasures in a hunt (similar to Phase 1’s `list` command).
- `view_treasure`: Shows details of a specific treasure (similar to Phase 1’s `view` command).
- `stop_monitor`: Requests the monitor to terminate and displays its termination state.
- `exit`: Exits the program (only if the monitor is not running).

Additional requirements:
- Communication with the monitor process uses **signals** (via `sigaction()`, not `signal()`).
- If `stop_monitor` is issued, commands are blocked until the monitor terminates (use `usleep()` for delay).
- Monitor termination is detected using `SIGCHLD`.

### Deliverables
- A working `treasure_hub` program.
- An updated `treasure_manager` program (if needed).

### Hints
- Use signals (e.g., `SIGUSR1`) to communicate commands to the monitor.
- Use files to share command details with the monitor process.

---

## Phase 3: Pipes, Redirects & External Integration

### Goal
Complete the system with inter-process communication and external tool integration.

### Requirements
Enhance the system with:
- The monitor process sends results back to the main process via a **pipe** instead of printing directly.
- A new `calculate_score` command in `treasure_hub` that:
  - Creates a process for each hunt to calculate user scores (sum of treasure values per user).
  - Uses an external program (written in C or as a script) to compute scores.
  - Retrieves the external program’s output via a pipe.

### Deliverables
- An updated `treasure_hub` program.
- The external program for calculating scores.
- An updated `treasure_manager` program (if needed).

---
