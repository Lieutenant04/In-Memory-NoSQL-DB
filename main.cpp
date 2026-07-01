#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <atomic>

// Include our core database structure
#include "Database.hpp"

// Persistence filename — used for save, load, and clear-timer
static const std::string DUMP_FILE = "dump.db";

using namespace nosqldb;

int main() {
    Database db;
    std::string line;

    std::cout << "--- In-Memory NoSQL Database Started ---\n";
    std::cout << "Available Commands:\n";
    std::cout << "  SET_INT <key> <value>\n";
    std::cout << "  SET_STR <key> <value>\n";
    std::cout << "  SET_DOUBLE <key> <value>\n";
    std::cout << "  SET_BOOL <key> <value>\n";
    std::cout << "  GET <key>\n";
    std::cout << "  DEL <key1> [key2] [key3] ...\n";
    std::cout << "  RENAME <old_key> <new_key>\n";
    std::cout << "  TYPE <key>\n";
    std::cout << "  KEYS\n";
    std::cout << "  EXISTS <key>\n";
    std::cout << "  COUNT\n";
    std::cout << "  LPUSH <key> <value>\n";
    std::cout << "  RPUSH <key> <value>\n";
    std::cout << "  LPOP <key>\n";
    std::cout << "  RPOP <key>\n";
    std::cout << "  LRANGE <key> <start> <stop>\n";
    std::cout << "  LLEN <key>\n";
    std::cout << "  CLEAR_TIMER <seconds>\n";
    std::cout << "  EXIT\n";
    std::cout << "----------------------------------------\n\n";

    // Load existing data from disk before accepting new commands
    auto loadResult = db.loadFromFile(DUMP_FILE);
    if (loadResult.fileFound) {
        std::cout << "SUCCESS: Database loaded from disk (" << DUMP_FILE << "). "
                  << loadResult.entriesLoaded << " entries loaded.\n";
        for (const auto& warning : loadResult.warnings) {
            std::cerr << "Warning: " << warning << "\n";
        }
    } else {
        std::cout << "Notice: No existing database found. Starting fresh.\n";
    }

    // Flag set by the timer callback so the main loop can print the message safely
    std::atomic<bool> timerFired{false};

    // Infinite loop to keep the server running
    while (true) {
        // Check if the background timer fired and print the notification synchronously
        if (timerFired.exchange(false)) {
            std::cout << "\nSUCCESS: Database and " << DUMP_FILE << " have been emptied by the timer.\n";
        }
        std::cout << "> ";
        
        // Read the entire line typed by the user
        if (!std::getline(std::cin, line)) {
            break; 
        }

        // Use stringstream to split the sentence into individual words
        std::stringstream ss(line);
        std::string word;
        std::vector<std::string> args;

        while (ss >> word) {
            args.push_back(word);
        }

        // If the user just pressed Enter without typing anything, ask again
        if (args.empty()) {
            continue;
        }

        // The first word is always the command
        std::string cmd = args[0];

        // Route the command to the correct Database method
        if (cmd == "EXIT") {
            std::cout << "Saving and shutting down database...\n";
            if (db.saveToFile(DUMP_FILE)) {
                std::cout << "SUCCESS: Database saved to disk (" << DUMP_FILE << ").\n";
            } else {
                std::cout << "Error: Could not save database to " << DUMP_FILE << ".\n";
            }
            break;
        }
        else if (cmd == "SET_INT" && args.size() >= 3) {
            try {
                int val = std::stoi(args[2]);
                if (db.setInt(args[1], val)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } catch (...) {
                std::cout << "Error: Value must be a valid integer.\n";
            }
        } 
        else if (cmd == "SET_STR" && args.size() >= 3) {
            // Join all arguments after the key to support values with spaces
            std::string val;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2) val += " ";
                val += args[i];
            }
            if (db.setString(args[1], val)) {
                std::cout << "OK\n";
            } else {
                std::cout << "Error: Key cannot be empty.\n";
            }
        } 
        else if (cmd == "GET" && args.size() >= 2) {
            if (auto val = db.get(args[1])) {
                std::cout << *val << "\n";
            } else {
                std::cout << "(nil) - Key not found\n";
            }
        } 
        else if (cmd == "DEL" && args.size() >= 2) {
            if (args.size() == 2) {
                // Single key: use the original remove for a simple message
                if (db.remove(args[1])) {
                    std::cout << "Deleted\n";
                } else {
                    std::cout << "(nil) - Key not found\n";
                }
            } else {
                // Multiple keys: collect them and bulk-delete
                std::vector<std::string> keysToDelete(args.begin() + 1, args.end());
                auto result = db.removeMultiple(keysToDelete);
                std::cout << "Deleted " << result.deleted << " key(s)\n";
                if (!result.notFound.empty()) {
                    std::cout << "Not found:";
                    for (const auto& k : result.notFound) {
                        std::cout << " " << k;
                    }
                    std::cout << "\n";
                }
            }
        }
        else if (cmd == "RENAME" && args.size() >= 3) {
            if (db.rename(args[1], args[2])) {
                std::cout << "OK\n";
            } else {
                std::cout << "Error: Source key not found, keys are empty, or keys are identical.\n";
            }
        }
        else if (cmd == "TYPE" && args.size() >= 2) {
            if (auto vt = db.type(args[1])) {
                std::cout << valueTypeToString(*vt) << "\n";
            } else {
                std::cout << "(nil) - Key not found\n";
            }
        }
        else if (cmd == "KEYS") {
            auto allKeys = db.keys();
            if (allKeys.empty()) {
                std::cout << "(empty)\n";
            } else {
                for (const auto& k : allKeys) {
                    std::cout << k << "\n";
                }
            }
        }
        else if (cmd == "EXISTS" && args.size() >= 2) {
            std::cout << (db.exists(args[1]) ? "1 (exists)" : "0 (not found)") << "\n";
        }
        else if (cmd == "COUNT") {
            std::cout << db.count() << "\n";
        }
        else if (cmd == "CLEAR_TIMER" && args.size() >= 2) {
            try {
                int seconds = std::stoi(args[1]);
                if (db.setClearTimer(seconds, DUMP_FILE, [&timerFired]() {
                        timerFired.store(true);
                    })) {
                    std::cout << "Timer set for " << seconds << " seconds. The database and " << DUMP_FILE << " will be emptied then.\n";
                } else {
                    std::cout << "Error: Seconds must be a positive integer.\n";
                }
            } catch (...) {
                std::cout << "Error: Seconds must be a valid integer.\n";
            }
        }
        else if (cmd == "SET_DOUBLE" && args.size() >= 3) {
            try {
                double val = std::stod(args[2]);
                if (db.setDouble(args[1], val)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } catch (...) {
                std::cout << "Error: Value must be a valid double.\n";
            }
        }
        else if (cmd == "SET_BOOL" && args.size() >= 3) {
            std::string val = args[2];
            if (val == "true" || val == "1") {
                if (db.setBool(args[1], true)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } else if (val == "false" || val == "0") {
                if (db.setBool(args[1], false)) {
                    std::cout << "OK\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } else {
                std::cout << "Error: Value must be 'true', 'false', '1', or '0'.\n";
            }
        }
        else if (cmd == "LPUSH" && args.size() >= 3) {
            std::string val;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2) val += " ";
                val += args[i];
            }
            int result = db.listPushLeft(args[1], val);
            if (result == -1) {
                if (db.exists(args[1])) {
                    std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } else {
                std::cout << "(integer) " << result << "\n";
            }
        }
        else if (cmd == "RPUSH" && args.size() >= 3) {
            std::string val;
            for (size_t i = 2; i < args.size(); ++i) {
                if (i > 2) val += " ";
                val += args[i];
            }
            int result = db.listPushRight(args[1], val);
            if (result == -1) {
                if (db.exists(args[1])) {
                    std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                } else {
                    std::cout << "Error: Key cannot be empty.\n";
                }
            } else {
                std::cout << "(integer) " << result << "\n";
            }
        }
        else if (cmd == "LPOP" && args.size() >= 2) {
            auto result = db.listPopLeft(args[1]);
            if (result.has_value()) {
                std::cout << *result << "\n";
            } else {
                if (db.exists(args[1])) {
                    auto t = db.type(args[1]);
                    if (t.has_value() && *t != ValueType::LIST) {
                        std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                    } else {
                        std::cout << "(nil)\n";
                    }
                } else {
                    std::cout << "(nil)\n";
                }
            }
        }
        else if (cmd == "RPOP" && args.size() >= 2) {
            auto result = db.listPopRight(args[1]);
            if (result.has_value()) {
                std::cout << *result << "\n";
            } else {
                if (db.exists(args[1])) {
                    auto t = db.type(args[1]);
                    if (t.has_value() && *t != ValueType::LIST) {
                        std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                    } else {
                        std::cout << "(nil)\n";
                    }
                } else {
                    std::cout << "(nil)\n";
                }
            }
        }
        else if (cmd == "LRANGE" && args.size() >= 4) {
            try {
                int start = std::stoi(args[2]);
                int stop = std::stoi(args[3]);
                auto result = db.listRange(args[1], start, stop);
                if (result.has_value()) {
                    if (result->empty()) {
                        std::cout << "(empty list)\n";
                    } else {
                        for (size_t i = 0; i < result->size(); ++i) {
                            std::cout << (i + 1) << ") " << (*result)[i] << "\n";
                        }
                    }
                } else {
                    if (db.exists(args[1])) {
                        std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                    } else {
                        std::cout << "(nil) - Key not found\n";
                    }
                }
            } catch (...) {
                std::cout << "Error: Start and stop must be valid integers.\n";
            }
        }
        else if (cmd == "LLEN" && args.size() >= 2) {
            auto result = db.listLength(args[1]);
            if (result.has_value()) {
                std::cout << "(integer) " << *result << "\n";
            } else {
                if (db.exists(args[1])) {
                    std::cout << "WRONGTYPE: Key holds a value that is not a list.\n";
                } else {
                    std::cout << "(nil) - Key not found\n";
                }
            }
        }
        else {
            std::cout << "Error: Invalid command or missing arguments.\n";
        }
    }

    return 0;
}