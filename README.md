# In-Memory NoSQL Database

A simple, thread-safe, in-memory NoSQL key-value database written in C++. The database allows storing integer and string values, querying them, deleting them, and saving the current state to a disk file for persistence.

## Features
- **In-Memory Store**: Fast key-value store built around `std::unordered_map`.
- **Thread Safety**: Operations on the database are protected with `std::mutex` for safe concurrent access.
- **Polymorphism**: Uses an interface (`IValue`) with concrete implementations (`IntValue`, `StringValue`) to store different data types under the same data structure.
- **Data Persistence**: Automatically loads existing data from `dump.db` upon startup and saves the current database state to the file when exiting the application cleanly.
- **Background Timer**: Includes a command to start a background thread that clears the database in-memory and on-disk after a specified number of seconds.

## Available Commands
When you run the application, you enter a CLI loop where you can type commands:

| Command | Description |
| --- | --- |
| `SET_INT <key> <value>` | Insert or update an integer value for the given key. |
| `SET_STR <key> <value>` | Insert or update a string value for the given key. |
| `GET <key>` | Retrieve and display the value associated with the key. Returns `(nil)` if not found. |
| `DEL <key>` | Delete the key-value pair from the database. |
| `CLEAR_TIMER <seconds>` | Spawns a background thread that waits for the specified number of seconds and then empties the database from memory and truncates `dump.db`. |
| `EXIT` | Saves all in-memory data to `dump.db` and shuts down the database CLI. |

## Example Usage

```text
--- In-Memory NoSQL Database Started ---
Available Commands:
  SET_INT <key> <value>
  SET_STR <key> <value>
  GET <key>
  DEL <key>
  CLEAR_TIMER <seconds>
  EXIT
----------------------------------------

> SET_INT score 150
OK
> SET_STR player_name Alice
OK
> GET score
150
> DEL score
Deleted
> GET score
(nil) - Key not found
> EXIT
Saving and shutting down database...
SUCCESS: Database saved to disk (dump.db).
```

## Build and Run

To compile the project, you need a C++14 (or newer) compatible compiler because of the use of `std::make_unique` and standard threading libraries.

### Compilation (e.g., using GCC)
```bash
g++ -std=c++14 main.cpp -pthread -o db_app
```

### Running
```bash
./db_app
```
(On Windows, you can just run `db_app.exe`)

## Project Structure

- `main.cpp` - Entry point containing the CLI application loop and command parsing.
- `Database.hpp` - The core database system. Manages the `unordered_map`, locking, persistence logic (`saveToFile`, `loadFromFile`), and timers.
- `IValue.hpp` - The base polymorphic interface for database values.
- `IntValue.hpp` - The integer value implementation of `IValue`.
- `StringValue.hpp` - The string value implementation of `IValue`.
- `dump.db` - The plaintext database dump file created on `EXIT` or modified during `CLEAR_TIMER`.
