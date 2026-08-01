#include <vector>
#include <print>
#include <iostream>
#include <cstring>
#include "api/PluginAPI.h"

// Turns out you can use non-c linkage in terms of classes for extern C,
// it just means other languages wont know how to read it

// native fn print_int(int32_t) -> void;
TINYLANG_EXPORT void print_int(const uint8_t* args, uint8_t* return_slot)
{
    const int32_t value = *reinterpret_cast<const int32_t*>(args);
    std::cout << value << "\n";
}

TINYLANG_EXPORT void print_char(const uint8_t* args, uint8_t*)
{
    const char8_t value = *reinterpret_cast<const char8_t*>(args);
    std::cout << static_cast<char>(value) << "\n";
}


// native fn tinylang_clock() -> float;
TINYLANG_EXPORT void tinylang_clock(const uint8_t* args, uint8_t* return_slot)
{
    constexpr float time = 42.0f; // Placeholder logic for now
    std::memcpy(return_slot, &time, sizeof(float));
}
