#include "project/ProjectConfig.h"


std::expected<std::unordered_map<std::string, ProjectConfig::DepType>, std::string> ProjectConfig::ParseDeps(
        const nlohmann::json::iterator& it, const std::filesystem::path& project_root)
{
    std::unordered_map<std::string, std::variant<GitDependency, LocalDependency>> deps;
    for(auto& [key, value] : it->items())
    {
        std::string dep_name = key;
        if(!value.is_object())
        {
            return std::unexpected("Expected object for dependency value");
        }

        if(value.size() != 1)
        {
            return std::unexpected("Only key 'path' or 'git' is allowed for local dependencies");
        }

        if(value.contains("path"))
        {
            using std::filesystem::path;
            path p = project_root / value.at("path").get<std::string>();
            if(!std::filesystem::is_directory(p))
            {
                return std::unexpected(std::format("Invalid file path '{}' for dependency '{}'", p.string(), dep_name));
            }
            path config_file = p / "tinylang.json";
            if(!exists(config_file))
            {
                return std::unexpected(std::format("No valid tinylang.json file in '{}' for dependency '{}'", config_file.string(), dep_name));
            }

            deps[dep_name] = LocalDependency{config_file};
        }
        else if(auto git = value.find("git"); git != value.end())
        {
            if(!git->is_object())
            {
                return std::unexpected(std::format("Git dependency '{}' must be a json object", dep_name));
            }

            auto url = git->find("url");
            if(url == git->end())
            {
                return std::unexpected(std::format("Git dependency '{}' must contain a url", dep_name));
            }

            const auto branch = git->value("branch", "main");
            const auto tag = GetOptional(git.value(), "tag");
            deps[dep_name] = GitDependency{url.value(), branch, tag};
        }
    }
    return deps;
}

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

                if(!j.contains("name")) return std::unexpected("No name field provided for project");
                if(!j.contains("version")) return std::unexpected("No version field provided for project");

                std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> plugin_mappings;
                if(auto it = j.find("plugins"); it != j.end() && it->is_object())
                {
                    for(auto& [key, value] : it->items())
                    {
                        if(value.is_string())
                        {
                            plugin_mappings[key] = value.get<std::string>();
                        }
                    }
                }

                std::unordered_map<std::string, DepType> deps;
                if(auto it = j.find("dependencies"); it != j.end() && it->is_object())
                {
                    auto deps_result = ParseDeps(it, current);
                    if(!deps_result) return std::unexpected(deps_result.error());

                    deps = std::move(deps_result.value());
                }

                return std::make_unique<ProjectConfig>
                (
                    std::move(current), std::move(j.at("name")),
                    std::move(j.at("version")), std::move(GetOptional(j, "entry")),
                    std::move(deps), std::move(plugin_mappings)
                );
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

    // implicitly add it as a dep (user still has to import in file, but not in the tinylang.json)
    if(module_name == "std")
    {
        if(const auto std_path = project_root / FormatOSPluginFilename("plugins/std_plugin");
            std::filesystem::exists(std_path))
        {
            return std_path;
        }
    }
    return project_root / FormatOSPluginFilename(module_name);
}
