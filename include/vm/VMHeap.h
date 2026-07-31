#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vm/ConstantValue.h"
#include "vm/Object.h"

class VMHeap
{
public:
    size_t gc_threshold = 256;

    template <typename T, typename... Args>
    T* Allocate(Args&&... args)
    {
        T* obj = new T(std::forward<Args>(args)...);
        objects.insert(obj);
        return obj;
    }

    void Sweep();
    void CollectGarbage(const std::vector<uint8_t>& vm_stack, const std::vector<uint8_t>& globals);
    void TraverseWorklist();

    void ScanMemory(const uint8_t* memory, size_t size);

    size_t Size() const { return objects.size(); }

private:
    std::unordered_set<Object*> objects{};
    std::vector<Object*> worklist{};

};
