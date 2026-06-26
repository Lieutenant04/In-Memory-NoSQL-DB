#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <mutex>
#include <thread>
#include <chrono>

// Include the base interface and concrete classes
#include "IValue.hpp"
#include "IntValue.hpp"
#include "StringValue.hpp"

class Database {
private:
    // The core data structure: a Hash Map storing unique pointers to IValue
    // Key: std::string (e.g., "score")
    // Value: std::unique_ptr<IValue> (Polymorphic pointer)
    std::unordered_map<std::string, std::unique_ptr<IValue>> store;
    mutable std::mutex dbMutex;

public:
    // Insert or update an integer value
    void setInt(const std::string& key, int value) {
        std::lock_guard<std::mutex> lock(dbMutex);
        // std::make_unique handles memory allocation safely
        store[key] = std::make_unique<IntValue>(value);
        std::cout << "OK\n";
    }

    // Insert or update a string value
    void setString(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<StringValue>(value);
        std::cout << "OK\n";
    }

    // Retrieve and print a value by its key
    void get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        // Use find() to search for the key without accidentally creating it
        auto it = store.find(key);
        
        // If the iterator doesn't point to the end, the key was found
        if (it != store.end()) {
            // it->second accesses the value (the pointer to IValue)
            it->second->print();
        } else {
            std::cout << "(nil) - Key not found\n";
        }
    }

    // Delete a key-value pair from the database
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(dbMutex);
        // erase() returns 1 if the element was removed, 0 if not found
        if (store.erase(key)) {
            std::cout << "Deleted\n";
        } else {
            std::cout << "(nil) - Key not found\n";
        }
    }

    // --- PERSISTENCE METHODS ---

    // Save the entire database to a text file
    void saveToFile(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::ofstream file(filename); // Open file for writing
        
        if (!file.is_open()) {
            std::cout << "Error: Could not open file for saving.\n";
            return;
        }

        // Iterate through the map and write each record to the file
        for (const auto& pair : store) {
            // Format: KEY TYPE VALUE (e.g., "player_score INT 150")
            file << pair.first << " " 
                 << pair.second->getType() << " " 
                 << pair.second->getAsString() << "\n";
        }

        file.close();
        std::cout << "SUCCESS: Database saved to disk (" << filename << ").\n";
    }

    // Load the database from a text file on startup
    void loadFromFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::ifstream file(filename); // Open file for reading
        
        // If the file doesn't exist (e.g., first run), just return quietly
        if (!file.is_open()) {
            std::cout << "Notice: No existing database found. Starting fresh.\n";
            return;
        }

        std::string key, type, value;
        
        // Read the file word by word
        // We read key, then type. Then we read the rest of the line as the value.
        while (file >> key >> type) {
            // std::ws skips any leading spaces before reading the value
            std::getline(file >> std::ws, value); 

            if (type == "INT") {
                // We use insert directly to bypass the "OK" print message of setInt
                store[key] = std::make_unique<IntValue>(std::stoi(value));
            } 
            else if (type == "STRING") {
                store[key] = std::make_unique<StringValue>(value);
            }
        }

        file.close();
        std::cout << "SUCCESS: Database loaded from disk (" << filename << ").\n";
    }

    // Empties the dump.db file and clears the memory store
    void emptyDatabaseFile() {
        std::lock_guard<std::mutex> lock(dbMutex);
        store.clear();
        std::ofstream file("dump.db", std::ios::trunc);
        file.close();
        std::cout << "\nSUCCESS: Database memory and dump.db have been emptied by the timer.\n> " << std::flush;
    }

    // Starts a background timer that empties the database file after a set time
    void setClearTimer(int seconds) {
        std::cout << "Timer set for " << seconds << " seconds. The database and dump.db will be emptied then.\n";
        std::thread([this, seconds]() {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            this->emptyDatabaseFile();
        }).detach();
    }
};

#endif