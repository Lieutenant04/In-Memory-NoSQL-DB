#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <unordered_map>
#include <string>
#include <memory>

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
#include "DoubleValue.hpp"
#include "BoolValue.hpp"
#include "ListValue.hpp"

namespace nosqldb {

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

    // Store a double value, returns false if key is empty
    bool setDouble(const std::string& key, double value) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<DoubleValue>(value);
        return true;
    }

    // Store a bool value, returns false if key is empty
    bool setBool(const std::string& key, bool value) {
        if (key.empty()) return false;
        std::lock_guard<std::mutex> lock(dbMutex);
        store[key] = std::make_unique<BoolValue>(value);
        return true;
    }

    // Push a value to the front of a list. Creates the list if key doesn't exist.
    // Returns -1 if key exists but is not a list, otherwise returns new list length.
    int listPushLeft(const std::string& key, const std::string& value) {
        if (key.empty()) return -1;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it != store.end()) {
            if (it->second->getType() != ValueType::LIST) return -1;
            auto* list = static_cast<ListValue*>(it->second.get());
            list->pushFront(value);
            return list->length();
        }
        auto newList = std::make_unique<ListValue>();
        newList->pushFront(value);
        int len = newList->length();
        store[key] = std::move(newList);
        return len;
    }

    // Push a value to the back of a list. Creates the list if key doesn't exist.
    // Returns -1 if key exists but is not a list, otherwise returns new list length.
    int listPushRight(const std::string& key, const std::string& value) {
        if (key.empty()) return -1;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it != store.end()) {
            if (it->second->getType() != ValueType::LIST) return -1;
            auto* list = static_cast<ListValue*>(it->second.get());
            list->pushBack(value);
            return list->length();
        }
        auto newList = std::make_unique<ListValue>();
        newList->pushBack(value);
        int len = newList->length();
        store[key] = std::move(newList);
        return len;
    }

    // Pop from the front of a list. Returns nullopt if key missing, not a list, or list empty.
    std::optional<std::string> listPopLeft(const std::string& key) {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it == store.end() || it->second->getType() != ValueType::LIST) return std::nullopt;
        auto* list = static_cast<ListValue*>(it->second.get());
        return list->popFront();
    }

    // Pop from the back of a list. Returns nullopt if key missing, not a list, or list empty.
    std::optional<std::string> listPopRight(const std::string& key) {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it == store.end() || it->second->getType() != ValueType::LIST) return std::nullopt;
        auto* list = static_cast<ListValue*>(it->second.get());
        return list->popBack();
    }

    // Return a range of elements from a list. Returns nullopt if key missing or not a list.
    std::optional<std::vector<std::string>> listRange(const std::string& key, int start, int stop) const {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it == store.end() || it->second->getType() != ValueType::LIST) return std::nullopt;
        auto* list = static_cast<const ListValue*>(it->second.get());
        return list->range(start, stop);
    }

    // Return the length of a list. Returns nullopt if key missing or not a list.
    std::optional<int> listLength(const std::string& key) const {
        if (key.empty()) return std::nullopt;
        std::lock_guard<std::mutex> lock(dbMutex);
        auto it = store.find(key);
        if (it == store.end() || it->second->getType() != ValueType::LIST) return std::nullopt;
        auto* list = static_cast<const ListValue*>(it->second.get());
        return list->length();
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

        // Clear existing data so a load replaces the store rather than silently merging
        store.clear();

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
            else if (type == "DOUBLE") {
                try {
                    store[key] = std::make_unique<DoubleValue>(std::stod(value));
                    result.entriesLoaded++;
                } catch (const std::exception& e) {
                    result.warnings.push_back("Skipping corrupt DOUBLE entry for key '" + key + "': " + e.what());
                }
            }
            else if (type == "BOOL") {
                if (value == "1") {
                    store[key] = std::make_unique<BoolValue>(true);
                    result.entriesLoaded++;
                } else if (value == "0") {
                    store[key] = std::make_unique<BoolValue>(false);
                    result.entriesLoaded++;
                } else {
                    result.warnings.push_back("Skipping corrupt BOOL entry for key '" + key + "': expected '0' or '1'");
                }
            }
            else if (type == "LIST") {
                store[key] = std::make_unique<ListValue>(ListValue::parseItems(value));
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
    // Returns false if the file could not be opened for truncation
    bool emptyDatabaseFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(dbMutex);
        store.clear();
        std::ofstream file(filename, std::ios::trunc);
        if (!file.is_open()) {
            return false;
        }
        file.close();
        return !file.fail();
    }

    // Starts a background timer that empties the database file after a set time
    // Accepts an optional callback to notify the caller when the timer fires
    // Returns false if seconds is not a positive value
    bool setClearTimer(int seconds, const std::string& filename,
                       std::function<void()> onComplete = nullptr) {
        if (seconds <= 0) return false;

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
        return true;
    }
};

} // namespace nosqldb

#endif