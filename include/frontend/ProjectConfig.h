#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "utils/Utils.h"

class ProjectConfig
{
public:
    std::filesystem::path project_root;
    std::string name;
    std::string version;

    std::filesystem::path ResolvePluginPath(std::string_view module_name) const;

    static std::expected<std::unique_ptr<ProjectConfig>, std::string> FindAndLoad(std::filesystem::path start_dir);

public:
    ProjectConfig() = default;

private:


    static std::string FormatOSPluginFilename(const std::filesystem::path& path);
    std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> plugin_mappings;
};
