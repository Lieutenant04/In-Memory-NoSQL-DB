#ifndef BOOLVALUE_HPP
#define BOOLVALUE_HPP

#include "IValue.hpp"

namespace nosqldb {

class BoolValue : public IValue {
private:
    bool data;

public:
    // Constructor initializing the data
    explicit BoolValue(bool val) : data(val) {}

    // Override to return formatted display string
    std::string toDisplayString() const override {
        return data ? "true" : "false";
    }

    // Override the getType method
    ValueType getType() const override {
        return ValueType::BOOL;
    }

    // Helper function to get the raw value if needed later
    bool getValue() const { return data; }

    // Convert the bool to string for serialization (compact: "1" or "0")
    std::string getAsString() const override {
        return data ? "1" : "0";
    }
};

} // namespace nosqldb

#endif
