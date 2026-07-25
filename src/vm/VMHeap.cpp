#include "vm/VMHeap.h"

void VMHeap::Sweep()
{
    for (auto first = objects.begin(), last = objects.end(); first != last;)
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

void VMHeap::CollectGarbage(const std::vector<uint8_t>& vm_stack) const
{
    for (size_t i = 0; i + sizeof(Object*) <= vm_stack.size(); i += 8)
    {
        Object* potential_object = nullptr;

        std::memcpy(&potential_object, &vm_stack[i], sizeof(Object*));
        if(objects.contains(potential_object))
        {
            potential_object->is_marked = true;
        }
    }
}
