#include "interpreter/NativeFunction.h"
#include <print>

namespace {
    constexpr auto PRINT_NAME = "Print";

    auto print_type = std::make_unique<FunctionType>(
        std::vector<const Type*>{PrimitiveType::String.get()},
        PrimitiveType::Void.get()
    );

    RuntimeValue NativePrint(const std::vector<RuntimeValue>& args) {
        std::println("{}", std::get<std::string>(args[0]));
        return std::monostate{};
    }

    std::vector<NativeFunction::NativeDef> registry{
        {PRINT_NAME, print_type.get(), std::make_shared<NativeFunctionWrapper>(NativePrint)}
    };
}

namespace NativeFunction {

    void RegisterTypes(SymbolTable& symbol_table)
    {
        for (const auto& [name, type, implementation] : registry) {
            symbol_table.DefineVariable({name, type});
        }
    }

    void RegisterImplementations(Environment* environment)
    {
        for (const auto& [name, type, implementation] : registry) {
            environment->Define(name, implementation);
        }
    }

}
