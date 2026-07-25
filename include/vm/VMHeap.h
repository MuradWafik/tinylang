#pragma once
#include <cstdint>
#include <unordered_set>
#include <vector>

#include "vm/Object.h"

class VMHeap
{
public:
    template <typename T, typename... Args>
    T* Allocate(Args&&... args)
    {
        T* obj = new T(std::forward<Args>(args)...);
        objects.insert(obj);
        return obj;
    }

    void Sweep();
    void CollectGarbage(const std::vector<uint8_t>& vm_stack) const;
private:
    std::unordered_set<Object*> objects;
};
