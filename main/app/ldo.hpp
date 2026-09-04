#pragma once
#include <functional>

class Ldo {
public:
    Ldo(int id);
    Ldo(int id, int volatage);
    ~Ldo();

    template <typename T>
    T as() const { return static_cast<T>(handle); }
    
private:
    void *handle = nullptr;
    std::function<void()> deleter;
};