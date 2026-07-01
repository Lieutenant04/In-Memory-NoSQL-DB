#ifndef DOUBLEVALUE_HPP
#define DOUBLEVALUE_HPP

#include "IValue.hpp"

namespace nosqldb {

class DoubleValue : public IValue {
private:
    double data;

    // Trim trailing zeros from a decimal string (e.g. "3.140000" → "3.14")
    static std::string trimTrailingZeros(std::string s) {
        auto dot = s.find('.');
        if (dot == std::string::npos) return s;
        auto last = s.find_last_not_of('0');
        if (last == dot) return s.substr(0, dot);  // remove trailing '.' too
        return s.substr(0, last + 1);
    }

public:
    // Constructor initializing the data
    explicit DoubleValue(double val) : data(val) {}

    // Override to return formatted display string
    std::string toDisplayString() const override {
        return trimTrailingZeros(std::to_string(data));
    }

    // Override the getType method
    ValueType getType() const override {
        return ValueType::DOUBLE;
    }

    // Helper function to get the raw value if needed later
    double getValue() const { return data; }

    // Convert the double to string for serialization
    std::string getAsString() const override {
        return trimTrailingZeros(std::to_string(data));
    }
};

} // namespace nosqldb

#endif
