# In-Memory NoSQL Database

A thread-safe, in-memory key-value database written in C++17. Supports integer and string value types, file-based persistence with corruption-resilient loading, background timers, and a clean separation between data logic and I/O.

## Features

- **In-Memory Store**: Fast key-value storage using `std::unordered_map`
- **Bulk Operations**: Delete multiple keys in a single command with per-key feedback
- **Thread Safety**: All operations protected with `std::mutex` for safe concurrent access
- **Polymorphism**: Interface-based design (`IValue` → `IntValue`, `StringValue`) for extensible value types
- **Robust Persistence**: Tab-separated file format with escape encoding and graceful handling of corrupt or malformed entries
- **Background Timer**: Schedule automatic database clearing with safe thread lifecycle management
- **Clean Architecture**: Database methods return values instead of printing — all I/O is handled by the CLI layer
- **Test Suite**: 29 automated tests covering CRUD, bulk delete, persistence round-trips, error handling, and timer behaviour

## Available Commands

| Command | Description |
| --- | --- |
| `SET_INT <key> <value>` | Store an integer value for the given key |
| `SET_STR <key> <value>` | Store a string value (supports spaces in values) |
| `GET <key>` | Retrieve the value associated with the key |
| `DEL <key1> [key2] ...` | Delete one or more key-value pairs |
| `RENAME <old_key> <new_key>` | Atomically rename a key (overwrites target if it exists) |
| `TYPE <key>` | Return the type of the value stored under a key (INT or STRING) |
| `KEYS` | List all keys in the database |
| `EXISTS <key>` | Check if a key exists (returns 1 or 0) |
| `COUNT` | Return the total number of stored entries |
| `CLEAR_TIMER <seconds>` | Schedule the database to be emptied after N seconds |
| `EXIT` | Save all data to disk and shut down |

## Example Usage

```text
--- In-Memory NoSQL Database Started ---
Available Commands:
  SET_INT <key> <value>
  SET_STR <key> <value>
  GET <key>
  DEL <key1> [key2] [key3] ...
  RENAME <old_key> <new_key>
  TYPE <key>
  KEYS
  EXISTS <key>
  COUNT
  CLEAR_TIMER <seconds>
  EXIT
----------------------------------------

Notice: No existing database found. Starting fresh.
> SET_INT score 150
OK
> SET_STR player_name Alice Smith
OK
> GET player_name
"Alice Smith"
> GET score
150
> KEYS
score
player_name
> EXISTS score
1 (exists)
> COUNT
2
> TYPE score
INT
> RENAME score points
OK
> GET points
150
> DEL points
Deleted
> SET_INT hp 100
OK
> SET_INT mp 50
OK
> DEL points hp ghost
Deleted 2 key(s)
Not found: ghost
> COUNT
2
> EXIT
Saving and shutting down database...
SUCCESS: Database saved to disk (dump.db).
```

## Build

### Option 1: CMake (Recommended)

```bash
cmake -B build
cmake --build build
```

### Option 2: Manual Compilation

```bash
# Main application
g++ -std=c++17 main.cpp -pthread -o db_app

# Test suite
g++ -std=c++17 tests.cpp -pthread -o db_tests
```

### Running

```bash
./db_app        # Linux/macOS
db_app.exe      # Windows
```

### Running Tests

```bash
# Via CMake + CTest
cd build && ctest --output-on-failure

# Or directly
./db_tests      # Linux/macOS
db_tests.exe    # Windows
```

## Project Structure

| File | Purpose |
| --- | --- |
| `main.cpp` | CLI application loop, command parsing, and all user-facing I/O |
| `Database.hpp` | Core database engine — data storage, persistence, timers (no I/O) |
| `IValue.hpp` | Base polymorphic interface for stored values |
| `IntValue.hpp` | Integer value implementation |
| `StringValue.hpp` | String value implementation |
| `tests.cpp` | Automated test suite (29 tests covering CRUD, bulk delete, persistence, timers) |
| `CMakeLists.txt` | CMake build configuration |

## Architecture

```
┌─────────────────────────────────────────────┐
│  main.cpp (CLI Layer)                       │
│  • Parses user commands                     │
│  • Handles ALL input/output (cout/cerr)     │
│  • Formats display strings for the user     │
├─────────────────────────────────────────────┤
│  Database.hpp (Engine Layer)                │
│  • Returns values (bool, string, vector...) │
│  • Zero cout/cerr — fully testable          │
│  • Thread-safe mutex on all operations      │
├─────────────────────────────────────────────┤
│  IValue / IntValue / StringValue            │
│  • Polymorphic value storage                │
│  • Type-safe serialization & display        │
└─────────────────────────────────────────────┘
```

## Persistence Format

Data is saved to `dump.db` as tab-separated records:

```
key1	INT	42
key2	STRING	Hello World
```

Special characters in keys and values are escaped to keep the format safe:

| Character | Escaped as |
| --- | --- |
| `\` | `\\` |
| Tab | `\t` |
| Newline | `\n` |

This format safely handles values containing spaces, tabs, and newlines. Malformed or corrupt entries are skipped with warnings on load.
