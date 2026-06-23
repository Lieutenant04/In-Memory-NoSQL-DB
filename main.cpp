#include <iostream>
#include <vector>
#include <memory> // Required for std::unique_ptr and std::make_unique

// Include our custom classes
#include "IntValue.hpp"
#include "StringValue.hpp"

int main() {
    // Create a vector that can hold pointers to ANY derived class of IValue
    std::vector<std::unique_ptr<IValue>> simpleDatabase;

    // Add an integer and a string into the exact same vector using polymorphism
    simpleDatabase.push_back(std::make_unique<IntValue>(100));
    simpleDatabase.push_back(std::make_unique<StringValue>("Hello ACS UPB"));

    // Iterate through the vector. 
    // C++ knows exactly which print() method to call at runtime (Dynamic Binding)
    for (const auto& element : simpleDatabase) {
        std::cout << "[" << element->getType() << "]: ";
        element->print();
    }

    // Memory is AUTOMATICALLY freed here because of std::unique_ptr
    return 0; 
}