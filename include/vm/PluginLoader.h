#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include "vm/RuntimeValue.h"

using NativeFuncPtr = RuntimeValue (*)(const std::vector<RuntimeValue>&);

class PluginLoader
{
public:
    PluginLoader() = default;
    ~PluginLoader();

    std::expected<NativeFuncPtr, std::string> LoadSymbol(const std::string& lib_path, const std::string& symbol_name);

private:
    std::unordered_map<std::string, void*> loaded_libraries;
};
