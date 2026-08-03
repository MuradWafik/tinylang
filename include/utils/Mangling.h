#pragma once

#include <format>
#include <string>
#include <string_view>

namespace Mangling
{
    inline std::string ModuleSymbol(std::string_view module_name, std::string_view symbol_name)
    {
        return std::format("{}::{}", module_name, symbol_name);
    }

    inline std::string MethodName(std::string_view type_name, std::string_view method_name)
    {
        return std::format("{}${}", type_name, method_name);
    }
}
