#pragma once
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <functional>

struct FunctionDeclaration;
struct NativeFunctionWrapper;

using RuntimeValue = std::variant<
    int,
    float,
    bool,
    std::string,
    const FunctionDeclaration*,
    std::shared_ptr<NativeFunctionWrapper>, // Uses a pointer to the wrapper
    std::monostate  // void/no value
>;


struct NativeFunctionWrapper {
    std::function<RuntimeValue(const std::vector<RuntimeValue>&)> func;
};