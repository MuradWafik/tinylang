#pragma once

#include "interpreter/Environment.h"
#include "interpreter/RuntimeValue.h"
#include "analysis/SemanticAnalyzer.h"
#include "analysis/Type.h"

namespace NativeFunction
{
    struct NativeDef
    {
        std::string name;
        FunctionType* type;
        std::shared_ptr<NativeFunctionWrapper> implementation;
        NativeDef(std::string  name, FunctionType* type, decltype(implementation) implementation) : name{std::move(name)}, type{type}, implementation{std::move(implementation)} {}
    };

    void RegisterTypes(SymbolTable& symbol_table);
    void RegisterImplementations(Environment* environment);
};
