#ifndef IVALUE_HPP
#define IVALUE_HPP

#include <string>

namespace nosqldb {

// Type-safe enum for value types — prevents typos in string comparisons
enum class ValueType { INT, STRING, DOUBLE, BOOL, LIST };

// Convert ValueType to its string representation for serialization
inline std::string valueTypeToString(ValueType type) {
    switch (type) {
        case ValueType::INT:    return "INT";
        case ValueType::STRING: return "STRING";
        case ValueType::DOUBLE: return "DOUBLE";
        case ValueType::BOOL:   return "BOOL";
        case ValueType::LIST:   return "LIST";
    }
    return "UNKNOWN";
}

class IValue {
public:
    // Virtual destructor is MANDATORY in OOP. 
    // Without it, deleting a derived class pointer will cause a memory leak.
    virtual ~IValue() = default;

    // Get a formatted display string (e.g., integers as "42", strings as "\"hello\"")
    virtual std::string toDisplayString() const = 0;

    // Pure virtual method to get the data type
    virtual ValueType getType() const = 0;

    // Pure virtual method to get the string representation for file saving
    virtual std::string getAsString() const = 0;
};

} // namespace nosqldb

#endif