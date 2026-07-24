#pragma once

#include <expected>
#include <string>
#include <unordered_map>
#include "vm/ConstantValue.h"

using NativeFuncPtr = void (*)(const uint8_t*, uint8_t*);

class PluginLoader
{
public:
    PluginLoader() = default;
    ~PluginLoader();

    std::expected<NativeFuncPtr, std::string> LoadSymbol(const std::string& lib_path, const std::string& symbol_name);

private:
    std::unordered_map<std::string, void*> loaded_libraries;
};
