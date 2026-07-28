#include "vm/PluginLoader.h"

#include <dlfcn.h>
#include <format>
#include <ranges>

PluginLoader::~PluginLoader()
{
    for (const auto& handle: loaded_libraries | std::views::values)
    {
        if(handle)
        {
            dlclose(handle);
        }
    }
    loaded_libraries.clear();
}

std::expected<NativeFuncPtr, std::string> PluginLoader::LoadSymbol(const std::string& lib_path, const std::string& symbol_name)
{
    void* handle = nullptr;
    if(const auto it = loaded_libraries.find(lib_path); it != loaded_libraries.end())
    {
        handle = it->second;
    }
    else
    {
        handle = dlopen(lib_path.c_str(), RTLD_LAZY);
        if(!handle)
        {
            return std::unexpected(std::format("Failed to load plugin library '{}': {}", lib_path, dlerror()));
        }
        loaded_libraries[lib_path] = handle;
    }

    dlerror(); // Clear any existing errors
    auto symbol = reinterpret_cast<NativeFuncPtr>(dlsym(handle, symbol_name.c_str()));
    if(const char* dlsym_error = dlerror())
    {
        return std::unexpected(std::format("Failed to find symbol '{}' in '{}': {}", symbol_name, lib_path, dlsym_error));
    }

    return symbol;
}
