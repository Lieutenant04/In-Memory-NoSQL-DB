#ifndef INTVALUE_HPP
#define INTVALUE_HPP

#include "IValue.hpp"
#include <iostream>

class IntValue : public IValue {
private:
    int data;

public:
    // Constructor initializing the data
    IntValue(int val) : data(val) {}

    // Override the print method from the base class
    void print() const override {
        std::cout << data << std::endl;
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