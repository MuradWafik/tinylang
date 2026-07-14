#pragma once

#include <print>
#include <utility>

#include "Environment.h"
#include "RuntimeValue.h"
#include "SemanticAnalyzer.h"
#include "Type.h"

namespace NativeFunction
{

struct NativeDef
{
    std::string name;
    FunctionType* type;
    std::shared_ptr<NativeFunctionWrapper> implementation;
    NativeDef(std::string  name, FunctionType* type, decltype(implementation) implementation) : name{std::move(name)}, type{type}, implementation{std::move(implementation)} {}
};

inline constinit auto PRINT_NAME = "Print";

inline auto print_type = std::make_unique<FunctionType>(
                                                        std::vector<const Type*>{PrimitiveType::String.get()},
                                                        PrimitiveType::Void.get()
                                                       );

inline RuntimeValue NativePrint(const std::vector<RuntimeValue>& args) {
    std::println("{}", std::get<std::string>(args[0]));
    return std::monostate{};
}


inline const std::vector<NativeDef>& GetRegistry() {
    static std::vector<NativeDef> registry{
        {PRINT_NAME, print_type.get(), std::make_shared<NativeFunctionWrapper>(NativePrint)}
    };
    return registry;
}

inline void RegisterTypes(SymbolTable& symbol_table)
{
    for (const auto& [name, type, implementation] : GetRegistry()) {
        symbol_table.DefineVariable({name, type});
    }
}

inline void RegisterImplementations(Environment* environment)
{
    for (const auto& [name, type, implementation] : GetRegistry()) {
        environment->Define(name, implementation);
    }
}

};

