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
#include <atomic>
#include <functional>
#include <utility>
#include <vector>

// Include the base interface and concrete classes
#include "IValue.hpp"
#include "IntValue.hpp"
#include "StringValue.hpp"

// Result of loading from file, separates data from I/O concerns
struct LoadResult {
    bool fileFound = false;
    int entriesLoaded = 0;
    std::vector<std::string> warnings;
};

class Database {
private:
    // The core data structure: a Hash Map storing unique pointers to IValue
    // Key: std::string (e.g., "score")
    // Value: std::unique_ptr<IValue> (Polymorphic pointer)
    std::unordered_map<std::string, std::unique_ptr<IValue>> store;
    mutable std::mutex dbMutex;
    std::thread timerThread;
    std::atomic<bool> stopTimer{false};

public:
    // Destructor ensures the timer thread is joined before the object is destroyed
    ~Database() {
        stopTimer = true;
        if (timerThread.joinable()) {
            timerThread.join();
        }
    }

    // Insert or update an integer value
    void setInt(const std::string& key, int value) {
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<IntValue>(value);
    }

    // Insert or update a string value
    void setString(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<StringValue>(value);
    }

    // Retrieve a value by its key
    // Returns {found, displayString} — caller handles output
    std::pair<bool, std::string> get(const std::string& key) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it != store.end()) {
            return {true, it->second->toDisplayString()};
        }
        return {false, ""};
    }

    // Delete a key-value pair, returns true if the key was found and removed
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.erase(key) > 0;
    }

    // List all keys in the store
    std::vector<std::string> keys() const {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::vector<std::string> result;
        result.reserve(store.size());
        for (const auto& pair : store) {
            result.push_back(pair.first);
        }
        return result;
    }

    // Check if a key exists
    bool exists(const std::string& key) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.count(key) > 0;
    }

    // Return the number of entries
    size_t count() const {
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.size();
    }

    // --- PERSISTENCE METHODS ---

    // Save the entire database to a text file, returns true on success
    bool saveToFile(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            return false;
        }

        for (const auto& pair : store) {
            // Tab-separated format: KEY\tTYPE\tVALUE
            file << pair.first << "\t" 
                 << pair.second->getType() << "\t" 
                 << pair.second->getAsString() << "\n";
        }

        file.close();
        return true;
    }

    // Load the database from a text file on startup
    LoadResult loadFromFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(dbMutex);
        LoadResult result;
        std::ifstream file(filename);
        
        if (!file.is_open()) {
            return result;
        }

        result.fileFound = true;
        std::string line, key, type, value;
        
        // Read tab-separated records: KEY\tTYPE\tVALUE
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            auto tab1 = line.find('\t');
            auto tab2 = (tab1 != std::string::npos) ? line.find('\t', tab1 + 1) : std::string::npos;
            
            if (tab1 == std::string::npos || tab2 == std::string::npos) {
                result.warnings.push_back("Skipping malformed line in " + filename);
                continue;
            }
            
            key   = line.substr(0, tab1);
            type  = line.substr(tab1 + 1, tab2 - tab1 - 1);
            value = line.substr(tab2 + 1);

            if (type == "INT") {
                try {
                    store[key] = std::make_unique<IntValue>(std::stoi(value));
                    result.entriesLoaded++;
                } catch (const std::exception& e) {
                    result.warnings.push_back("Skipping corrupt INT entry for key '" + key + "': " + e.what());
                }
            } 
            else if (type == "STRING") {
                store[key] = std::make_unique<StringValue>(value);
                result.entriesLoaded++;
            }
            else {
                result.warnings.push_back("Unknown type '" + type + "' for key '" + key + "'");
            }
        }

        file.close();
        return result;
    }

    // Empties the database file and clears the memory store
    void emptyDatabaseFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(dbMutex);
        store.clear();
        std::ofstream file(filename, std::ios::trunc);
        file.close();
    }

    // Starts a background timer that empties the database file after a set time
    // Accepts an optional callback to notify the caller when the timer fires
    void setClearTimer(int seconds, const std::string& filename,
                       std::function<void()> onComplete = nullptr) {
        // Cancel any existing timer first
        stopTimer = true;
        if (timerThread.joinable()) {
            timerThread.join();
        }
        stopTimer = false;

        timerThread = std::thread([this, seconds, filename, onComplete]() {
            for (int i = 0; i < seconds * 10; ++i) {
                if (stopTimer.load()) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if (!stopTimer.load()) {
                this->emptyDatabaseFile(filename);
                if (onComplete) onComplete();
            }
        });
    }
};

#endif