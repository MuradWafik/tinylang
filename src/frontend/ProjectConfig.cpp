#include "frontend/ProjectConfig.h"


std::expected<std::unique_ptr<ProjectConfig>, std::string> ProjectConfig::FindAndLoad(std::filesystem::path start_dir)
{
    std::filesystem::path current = std::filesystem::absolute(start_dir);

    while(true)
    {
        if(auto config_path = current / "tinylang.json"; std::filesystem::exists(config_path))
        {
            try
            {
                std::ifstream file(config_path);
                nlohmann::json j;
                file >> j;

                auto config = std::make_unique<ProjectConfig>();;
                config->project_root = current;
                config->name = j.value("name", "unnamed");
                config->version = j.value("version", "0.1.0");

                if(j.contains("plugins") && j["plugins"].is_object())
                {
                    for(auto& [key, value] : j["plugins"].items())
                    {
                        if(value.is_string())
                        {
                            config->plugin_mappings[key] = value.get<std::string>();
                        }
                    }
                }
                return config;
            }
            catch(const std::exception& e)
            {
                return std::unexpected(e.what());
            }
            catch(...)
            {
                return std::unexpected("Unknown error encountered parsing project config");
            }
        }
        // keep checking parent dirs looking
        if(current == current.parent_path()) break;
        current = current.parent_path();
    }
    return std::unexpected("tinylang.json config file found");
}

std::string ProjectConfig::FormatOSPluginFilename(const std::filesystem::path& path)
{
    std::string filename = path.filename().string();
    const std::filesystem::path dir = path.parent_path();

    // If it already has an explicit extension, leave it untouched
    if(path.has_extension())
    {
        return path.string();
    }

#if defined(_WIN32)
        std::string os_name = filename + ".dll";
#elif defined(__APPLE__)
        std::string os_name = (filename.starts_with("lib") ? "" : "lib") + filename + ".dylib";
#else
    std::string os_name = (filename.starts_with("lib") ? "" : "lib") + filename + ".so";
#endif

    return (dir / os_name).string();
}

std::filesystem::path ProjectConfig::ResolvePluginPath(const std::string_view module_name) const
{
    if(const auto it = plugin_mappings.find(module_name); it != plugin_mappings.end())
    {
        return project_root / FormatOSPluginFilename(it->second);
    }
    return project_root / FormatOSPluginFilename(module_name);
}
