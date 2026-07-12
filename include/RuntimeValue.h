#pragma once
#include <string>
#include <variant>

using RuntimeValue = std::variant
<
    int,
    float,
    bool,
    std::string,
    std::monostate  // void/no value
>;
