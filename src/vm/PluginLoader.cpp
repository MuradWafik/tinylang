#include "vm/PluginLoader.h"
#include "utils/Utils.h"

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

std::expected<NativeFn, std::string> PluginLoader::LoadSymbol(const std::string& lib_path, const std::string& symbol_name)
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
            const auto exe_dir = GetExecutablePath().parent_path();
            const auto exe_filename = std::filesystem::path(lib_path).filename();

            std::vector<std::filesystem::path> candidate_paths = {
                exe_dir / exe_filename,
                exe_dir / "plugins" / exe_filename,
                exe_dir.parent_path() / exe_filename,
                exe_dir.parent_path() / "lib" / exe_filename,
                exe_dir.parent_path() / "lib" / "tinylang" / exe_filename,
            };

            if(const char* home = std::getenv("HOME"))
            {
                candidate_paths.push_back(std::filesystem::path(home) / ".tinylang" / "std" / exe_filename);
                candidate_paths.push_back(std::filesystem::path(home) / ".tinylang" / "plugins" / exe_filename);
            }

            for(const auto& cand : candidate_paths)
            {
                handle = dlopen(cand.c_str(), RTLD_LAZY);
                if(handle) break;
            }
        }
        if(!handle)
        {
            return std::unexpected(std::format("Failed to load plugin library '{}': {}", lib_path, dlerror()));
        }
        loaded_libraries[lib_path] = handle;
    }

    dlerror(); // Clear any existing errors
    auto symbol = reinterpret_cast<NativeFn>(dlsym(handle, symbol_name.c_str()));
    if(const char* dlsym_error = dlerror())
    {
        return std::unexpected(std::format("Failed to find symbol '{}' in '{}': {}", symbol_name, lib_path, dlsym_error));
    }

    return symbol;
}
