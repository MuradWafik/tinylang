#include "vm/VMHeap.h"

#include <ranges>
#include <unordered_map>

void VMHeap::Sweep()
{
    for(auto first = objects.begin(), last = objects.end(); first != last;)
    {
        if((*first)->is_marked)
        {
            (*first)->is_marked = false;
            ++first;
        }
        else
        {
            delete *first;
            first = objects.erase(first);
        }
    }
}

void VMHeap::CollectGarbage(const std::vector<uint8_t>& vm_stack, const std::vector<uint8_t>& globals)
{
    ScanMemory(vm_stack.data(), vm_stack.size());
    ScanMemory(globals.data(), globals.size());

    TraverseWorklist();
    Sweep();
}

void VMHeap::TraverseWorklist()
{
    while(!worklist.empty())
    {
        Object* obj = worklist.back();
        worklist.pop_back();
        if(const auto* arr = dynamic_cast<Array*>(obj))
        {
            ScanMemory(arr->elements, arr->size);
        }
        else if(const auto* struct_obj = dynamic_cast<Struct*>(obj))
        {
            ScanMemory(struct_obj->fields, struct_obj->size);
        }
    }
}

void VMHeap::ScanMemory(const uint8_t* memory, const size_t size)
{
    if(!memory || size < sizeof(Object*)) return;
    for(size_t i = 0; i + sizeof(Object*) <= size; i += 8)
    {
        Object* potential_object = nullptr;
        std::memcpy(&potential_object, &memory[i], sizeof(Object*));

        if(objects.contains(potential_object))
        {
            if(!potential_object->is_marked)
            {
                potential_object->is_marked = true;
                worklist.push_back(potential_object);
            }
        }
    }
}
