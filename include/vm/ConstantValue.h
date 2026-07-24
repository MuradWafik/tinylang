#pragma once

#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

class Chunk;
struct FunctionObject;

using ConstantValue = std::variant<
    int,
    float,
    bool,
    std::string,
    std::shared_ptr<FunctionObject>,
    std::monostate  // void/no value
>;

using NativeFn = void (*)(const uint8_t* args, uint8_t* return_slot);

struct FunctionObject
{
    std::string name;
    size_t num_args = 0;
    std::unique_ptr<Chunk> chunk{nullptr};

    uint8_t return_bytes = 0; // how many bytes to push after execution
    NativeFn native_fn{nullptr};
    [[nodiscard]] bool is_native() const { return native_fn != nullptr; }

    FunctionObject(std::string name, size_t num_args, std::unique_ptr<Chunk> chunk);
    FunctionObject(std::string name, size_t num_args, uint8_t return_bytes, NativeFn native_fn);
    ~FunctionObject();
};
/*
 * Initially had FunctionObject stored as a unique_ptr but gemini says that won't work, explanation as a reminder why below
 *
 * In a Virtual Machine, you are constantly duplicating values. You pass functions as callbacks,
 * you assign them to multiple variables, etc. If the RuntimeValue held a unique_ptr,
 * the C++ compiler would physically prevent you from duplicating the RuntimeValue.
 *
 */


inline std::string ToString(const ConstantValue& value)
{
    return std::visit([]<typename T0>(T0&& arg) -> std::string
    {
        using T = std::decay_t<T0>;
        if constexpr(std::is_same_v<T, int>) return std::to_string(arg);
        else if constexpr(std::is_same_v<T, float>) return std::to_string(arg);
        else if constexpr(std::is_same_v<T, bool>) return arg ? "true" : "false";
        else if constexpr(std::is_same_v<T, std::string>) return arg;
        else if constexpr(std::is_same_v<T, std::shared_ptr<FunctionObject>>) return std::format("<fn {}>", arg->name);
        else if constexpr(std::is_same_v<T, std::monostate>) return "void";
        else return "unknown";
    },
    value);
}

template <>
struct std::formatter<ConstantValue> : std::formatter<std::string>
{
    auto format(const ConstantValue& val, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(ToString(val), ctx);
    }
};
