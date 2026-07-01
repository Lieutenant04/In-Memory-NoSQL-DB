#ifndef LISTVALUE_HPP
#define LISTVALUE_HPP

#include "IValue.hpp"
#include <vector>
#include <optional>
#include <algorithm>

namespace nosqldb {

class ListValue : public IValue {
private:
    std::vector<std::string> items;

public:
    // Constructor initializing the list (defaults to empty)
    explicit ListValue(std::vector<std::string> items = {}) : items(std::move(items)) {}

    // Override to return formatted display string
    std::string toDisplayString() const override {
        if (items.empty()) return "(empty list)";
        std::string result = "[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) result += ", ";
            result += items[i];
        }
        result += "]";
        return result;
    }

    // Override the getType method
    ValueType getType() const override {
        return ValueType::LIST;
    }

    // Helper function to get the raw value if needed later
    const std::vector<std::string>& getValue() const { return items; }

    // Convert to comma-separated string with escaping for serialization
    // Escapes: backslash → \\, comma → \,
    std::string getAsString() const override {
        std::string result;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) result += ",";
            for (char c : items[i]) {
                if (c == '\\')     result += "\\\\";
                else if (c == ',') result += "\\,";
                else               result.push_back(c);
            }
        }
        return result;
    }

    // Insert a value at the beginning of the list
    void pushFront(const std::string& val) {
        items.insert(items.begin(), val);
    }

    // Insert a value at the end of the list
    void pushBack(const std::string& val) {
        items.push_back(val);
    }

    // Remove and return the front element, or nullopt if empty
    std::optional<std::string> popFront() {
        if (items.empty()) return std::nullopt;
        std::string val = items.front();
        items.erase(items.begin());
        return val;
    }

    // Remove and return the back element, or nullopt if empty
    std::optional<std::string> popBack() {
        if (items.empty()) return std::nullopt;
        std::string val = items.back();
        items.pop_back();
        return val;
    }

    // Return a slice of the list with negative index support (-1 = last)
    std::vector<std::string> range(int start, int stop) const {
        int n = static_cast<int>(items.size());
        if (n == 0) return {};

        // Convert negative indices to positive
        if (start < 0) start = n + start;
        if (stop < 0)  stop  = n + stop;

        // Clamp indices to valid range
        start = std::max(0, std::min(start, n - 1));
        stop  = std::max(0, std::min(stop,  n - 1));

        if (start > stop) return {};
        return std::vector<std::string>(items.begin() + start, items.begin() + stop + 1);
    }

    // Return the number of elements in the list
    int length() const {
        return static_cast<int>(items.size());
    }

    // Deserialize a comma-separated string back into a list (undoes escaping)
    static std::vector<std::string> parseItems(const std::string& serialized) {
        if (serialized.empty()) return {};
        std::vector<std::string> result;
        std::string current;
        for (size_t i = 0; i < serialized.size(); ++i) {
            if (serialized[i] == '\\' && i + 1 < serialized.size()) {
                char next = serialized[i + 1];
                if (next == '\\') { current.push_back('\\'); ++i; }
                else if (next == ',') { current.push_back(','); ++i; }
                else current.push_back(serialized[i]);
            }
            else if (serialized[i] == ',') {
                result.push_back(current);
                current.clear();
            }
            else {
                current.push_back(serialized[i]);
            }
        }
        result.push_back(current);
        return result;
    }
};

} // namespace nosqldb

#endif
