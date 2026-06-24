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
    std::cout << "  EXIT\n";
    std::cout << "----------------------------------------\n\n";

    // Load existing data from disk before accepting new commands
    db.loadFromFile("dump.db");

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
            // Save all data to disk right before exiting
            db.saveToFile("dump.db");
            break; // Exit the while loop
        }
        else if (cmd == "SET_INT" && args.size() >= 3) {
            try {
                // Convert the 3rd word (string) into an integer
                int val = std::stoi(args[2]);
                db.setInt(args[1], val);
            } catch (...) {
                // Catch error if the user typed a word instead of a number
                std::cout << "Error: Value must be a valid integer.\n";
            }
        } 
        else if (cmd == "SET_STR" && args.size() >= 3) {
            db.setString(args[1], args[2]);
        } 
        else if (cmd == "GET" && args.size() >= 2) {
            db.get(args[1]);
        } 
        else if (cmd == "DEL" && args.size() >= 2) {
            db.remove(args[1]);
        } 
        else {
            std::cout << "Error: Invalid command or missing arguments.\n";
        }
    }

    return 0;
}