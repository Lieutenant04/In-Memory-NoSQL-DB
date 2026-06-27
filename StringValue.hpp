#ifndef STRINGVALUE_HPP
#define STRINGVALUE_HPP

#include "IValue.hpp"
#include <string>

class StringValue : public IValue {
private:
    std::string data;

public:
    // Constructor initializing the string data
    StringValue(const std::string& val) : data(val) {}

    std::string toDisplayString() const override {
        return "\"" + data + "\"";
    }

    ValueType getType() const override {
        return ValueType::STRING;
    }

    std::string getValue() const { return data; }

    // Return the string directly for serialization
    std::string getAsString() const override {
        return data;
    }
};

#endif