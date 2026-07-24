#include <vector>
#include <print>
#include "api/PluginAPI.h"

// Turns out you can use non-c linkage in terms of classes for extern C,
// it just means other languages wont know how to read it

// native fn print_int(value: int) -> int;
TINYLANG_EXPORT RuntimeValue print_int(const std::vector<RuntimeValue>& args) {
    if (!args.empty() && std::holds_alternative<int>(args[0])) {
        std::println("{}", std::get<int>(args[0]));
    }
    return std::monostate{};
}

// native fn tinylang_clock() -> float;
TINYLANG_EXPORT RuntimeValue tinylang_clock(const std::vector<RuntimeValue>& args) {
    // Placeholder logic for now
    return 42.0f;
}
