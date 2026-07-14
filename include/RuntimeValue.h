#pragma once
#include <string>
#include <variant>

struct FunctionDeclaration;



using RuntimeValue = std::variant
<
    int,
    float,
    bool,
    std::string,
    const FunctionDeclaration*,
    std::monostate  // void/no value
>;
