#pragma once
#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include <format>

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

// Helper function to easily print RuntimeValues
inline std::string ToString(const RuntimeValue& value) {
    return std::visit([]<typename T0>(T0&& arg) -> std::string
    {
        using T = std::decay_t<T0>;
        if constexpr(std::is_same_v<T, int>) return std::to_string(arg);
        else if constexpr(std::is_same_v<T, float>) return std::to_string(arg);
        else if constexpr(std::is_same_v<T, bool>) return arg ? "true" : "false";
        else if constexpr(std::is_same_v<T, std::string>) return arg;
        else if constexpr(std::is_same_v<T, const FunctionDeclaration*>) return "<fn>";
        else if constexpr(std::is_same_v<T, std::shared_ptr<NativeFunctionWrapper>>) return "<native fn>";
        else if constexpr(std::is_same_v<T, std::monostate>) return "void";
        else return "unknown";
    },
    value);
}

template <>
struct std::formatter<RuntimeValue> : std::formatter<std::string>
{
    auto format(const RuntimeValue& val, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(ToString(val), ctx);
    }
};