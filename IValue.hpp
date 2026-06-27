#ifndef IVALUE_HPP
#define IVALUE_HPP

#include <string>

class IValue {
public:
    // Virtual destructor is MANDATORY in OOP. 
    // Without it, deleting a derived class pointer will cause a memory leak.
    virtual ~IValue() = default;

    // Get a formatted display string (e.g., integers as "42", strings as "\"hello\"")
    virtual std::string toDisplayString() const = 0;

    // Pure virtual method to get the data type (e.g., "INT", "STRING")
    virtual std::string getType() const = 0;

    // Pure virtual method to get the string representation for file saving
    virtual std::string getAsString() const = 0;
};

#endif