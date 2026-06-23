#ifndef STRINGVALUE_HPP
#define STRINGVALUE_HPP

#include "IValue.hpp"
#include <iostream>
#include <string>

class StringValue : public IValue {
private:
    std::string data;

public:
    // Constructor initializing the string data
    StringValue(const std::string& val) : data(val) {}

    void print() const override {
        std::cout << "\"" << data << "\"" << std::endl;
    }

    std::string getType() const override {
        return "STRING";
    }

    std::string getValue() const { return data; }
};

#endif