#ifndef IVALUE_HPP
#define IVALUE_HPP

#include <string>

class IValue {
public:
    // Virtual destructor is MANDATORY in OOP. 
    // Without it, deleting a derived class pointer will cause a memory leak.
    virtual ~IValue() = default;

    // Pure virtual method to print the value to the console
    virtual void print() const = 0;

    // Pure virtual method to get the data type (e.g., "INT", "STRING")
    virtual std::string getType() const = 0;

    // Pure virtual method to get the string representation for file saving
    virtual std::string getAsString() const = 0;
};

#endif