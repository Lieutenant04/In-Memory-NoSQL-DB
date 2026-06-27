#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// Include our core database structure
#include "Database.hpp"

int main() {
    Database db;
    std::string line;

    std::cout << "--- In-Memory NoSQL Database Started ---\n";
    std::cout << "Available Commands:\n";
    std::cout << "  SET_INT <key> <value>\n";
    std::cout << "  SET_STR <key> <value>\n";
    std::cout << "  GET <key>\n";
    std::cout << "  DEL <key>\n";
    std::cout << "  KEYS\n";
    std::cout << "  EXISTS <key>\n";
    std::cout << "  COUNT\n";
    std::cout << "  CLEAR_TIMER <seconds>\n";
    std::cout << "  EXIT\n";
    std::cout << "----------------------------------------\n\n";

    // Load existing data from disk before accepting new commands
    auto loadResult = db.loadFromFile("dump.db");
    if (loadResult.fileFound) {
        std::cout << "SUCCESS: Database loaded from disk (dump.db). "
                  << loadResult.entriesLoaded << " entries loaded.\n";
        for (const auto& warning : loadResult.warnings) {
            std::cerr << "Warning: " << warning << "\n";
        }
    } else {
        std::cout << "Notice: No existing database found. Starting fresh.\n";
    }

    // Infinite loop to keep the server running
    while (true) {
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
            if (db.saveToFile("dump.db")) {
                std::cout << "SUCCESS: Database saved to disk (dump.db).\n";
            } else {
                std::cout << "Error: Could not save database to dump.db.\n";
            }
            break;
        }
        else if (cmd == "SET_INT" && args.size() >= 3) {
            try {
                int val = std::stoi(args[2]);
                db.setInt(args[1], val);
                std::cout << "OK\n";
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
            db.setString(args[1], val);
            std::cout << "OK\n";
        } 
        else if (cmd == "GET" && args.size() >= 2) {
            auto result = db.get(args[1]);
            if (result.first) {
                std::cout << result.second << "\n";
            } else {
                std::cout << "(nil) - Key not found\n";
            }
        } 
        else if (cmd == "DEL" && args.size() >= 2) {
            if (db.remove(args[1])) {
                std::cout << "Deleted\n";
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
                std::cout << "Timer set for " << seconds << " seconds. The database and dump.db will be emptied then.\n";
                db.setClearTimer(seconds, "dump.db", []() {
                    std::cout << "\nSUCCESS: Database and dump.db have been emptied by the timer.\n" << std::flush;
                });
            } catch (...) {
                std::cout << "Error: Seconds must be a valid integer.\n";
            }
        }
        else {
            std::cout << "Error: Invalid command or missing arguments.\n";
        }
    }

    return 0;
}