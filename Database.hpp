#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>

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

public:
    // Insert or update an integer value
    void setInt(const std::string& key, int value) {
        // std::make_unique handles memory allocation safely
        store[key] = std::make_unique<IntValue>(value);
        std::cout << "OK\n";
    }

    // Insert or update a string value
    void setString(const std::string& key, const std::string& value) {
        store[key] = std::make_unique<StringValue>(value);
        std::cout << "OK\n";
    }

    // Retrieve and print a value by its key
    void get(const std::string& key) const {
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
        // erase() returns 1 if the element was removed, 0 if not found
        if (store.erase(key)) {
            std::cout << "Deleted\n";
        } else {
            std::cout << "(nil) - Key not found\n";
        }
    }
};

#endif