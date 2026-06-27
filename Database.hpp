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
#include <optional>
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

    // Escape special characters so tab-separated format stays intact
    // Encodes: \ → \\, tab → \t, newline → \n
    static std::string escape(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            if (c == '\\')     result += "\\\\";
            else if (c == '\t') result += "\\t";
            else if (c == '\n') result += "\\n";
            else                result.push_back(c);
        }
        return result;
    }

    // Reverse the escape encoding when reading from the dump file
    static std::string unescape(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char next = s[i + 1];
                if (next == '\\') { result.push_back('\\'); ++i; }
                else if (next == 't') { result.push_back('\t'); ++i; }
                else if (next == 'n') { result.push_back('\n'); ++i; }
                else result.push_back(s[i]);
            } else {
                result.push_back(s[i]);
            }
        }
        return result;
    }

public:
    // Destructor ensures the timer thread is joined before the object is destroyed
    ~Database() {
        stopTimer = true;
        if (timerThread.joinable()) {
            timerThread.join();
        }
    }

    // Database manages a mutex and a thread — copying or moving makes no sense
    Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    // Insert or update an integer value, returns false if key is empty
    bool setInt(const std::string& key, int value) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<IntValue>(value);
        return true;
    }

    // Insert or update a string value, returns false if key is empty
    bool setString(const std::string& key, const std::string& value) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<StringValue>(value);
        return true;
    }

    // Retrieve a value by its key
    // Returns the display string if found, or std::nullopt if the key doesn't exist
    std::optional<std::string> get(const std::string& key) const {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it != store.end()) {
            return it->second->toDisplayString();
        }
        return std::nullopt;
    }

    // Delete a key-value pair, returns true if the key was found and removed
    bool remove(const std::string& key) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.erase(key) > 0;
    }

    // Result of a bulk delete operation
    struct RemoveResult {
        int deleted = 0;
        std::vector<std::string> notFound;
    };

    // Delete multiple keys in one operation, returns how many were deleted and which were missing
    RemoveResult removeMultiple(const std::vector<std::string>& keys) {
        std::lock_guard<std::mutex> lock(dbMutex);
        RemoveResult result;
        for (const auto& key : keys) {
            if (key.empty()) continue;
            if (store.erase(key) > 0) {
                result.deleted++;
            } else {
                result.notFound.push_back(key);
            }
        }
        return result;
    }

    // Atomically rename a key. Overwrites newKey if it already exists (Redis behaviour).
    // Returns false if oldKey doesn't exist or either key is empty.
    bool rename(const std::string& oldKey, const std::string& newKey) {
        if (oldKey.empty() || newKey.empty() || oldKey == newKey) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(oldKey);
        if (it == store.end()) return false;
        store.erase(newKey);  // remove target if it exists
        auto node = store.extract(it);
        node.key() = newKey;
        store.insert(std::move(node));
        return true;
    }

    // Return the type of the value stored under key, or nullopt if key doesn't exist
    std::optional<ValueType> type(const std::string& key) const {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it != store.end()) {
            return it->second->getType();
        }
        return std::nullopt;
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
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.count(key) > 0;
    }

    // Return the number of entries
    size_t count() const {
        std::lock_guard<std::mutex> lock(dbMutex);
        return store.size();
    }

    // --- PERSISTENCE METHODS ---

    // Save the entire database to a text file
    // Returns true only if every write succeeded (catches disk-full, I/O errors)
    bool saveToFile(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(dbMutex);
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            return false;
        }

        for (const auto& pair : store) {
            // Tab-separated format: KEY\tTYPE\tVALUE (with escaped special chars)
            file << escape(pair.first) << "\t" 
                 << valueTypeToString(pair.second->getType()) << "\t" 
                 << escape(pair.second->getAsString()) << "\n";
        }

        file.close();
        return !file.fail();  // catches write errors (disk full, I/O failure, etc.)
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
            
            key   = unescape(line.substr(0, tab1));
            type  = line.substr(tab1 + 1, tab2 - tab1 - 1);
            value = unescape(line.substr(tab2 + 1));

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