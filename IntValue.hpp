#ifndef INTVALUE_HPP
#define INTVALUE_HPP

#include "IValue.hpp"

class IntValue : public IValue {
private:
    int data;

public:
    // Constructor initializing the data
    IntValue(int val) : data(val) {}

    // Override to return formatted display string
    std::string toDisplayString() const override {
        return std::to_string(data);
    }

    // Override the getType method
    std::string getType() const override {
        return "INT";
    }

    // Helper function to get the raw value if needed later
    int getValue() const { return data; }

    // Convert the integer to string for serialization
    std::string getAsString() const override {
        return std::to_string(data);
    }
};

#endif