#include <iostream>
#pragma once
class MyClass {
public:
    MyClass(int value) : value_(value) {
        std::cout << "MyClass ctor" << std::endl;
    }
    void myFunction(int count, const std::string& label) const {
        std::cout << "Hello, World!" << std::endl;
    }
    virtual MyClass* clone() = delete;
};
