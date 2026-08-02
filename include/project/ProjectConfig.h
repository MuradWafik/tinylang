#pragma once
#include <expected>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>

#include "utils/Utils.h"



class ProjectConfig
{
public:
    struct GitDependency
    {
        std::string url;
        std::string branch;
        std::optional<std::string> tag; // specify its optional even if strings can be empty
    };

    struct LocalDependency
    {
        std::filesystem::path path;
    };

    using DepType = std::variant<GitDependency, LocalDependency>;

    std::filesystem::path project_root;
    std::string name;
    std::string version;
    std::optional<std::filesystem::path> entry_point;
    std::unordered_map<std::string, std::variant<GitDependency, LocalDependency>> dependencies;
    std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> plugin_mappings;

    std::filesystem::path ResolvePluginPath(std::string_view module_name) const;

    static std::expected<std::unique_ptr<ProjectConfig>, std::string> FindAndLoad(std::filesystem::path start_dir);

    ProjectConfig(
        std::filesystem::path project_root, std::string name, std::string version,
        std::optional<std::filesystem::path> entry_point,
        std::unordered_map<std::string, std::variant<GitDependency, LocalDependency>> dependencies,
        std::unordered_map<std::string, std::string, StringHash, std::equal_to<>> plugin_mappings
    )
    : project_root(std::move(project_root)), name(std::move(name)), version(std::move(version)),
      entry_point(std::move(entry_point)), dependencies(std::move(dependencies)), plugin_mappings(std::move(plugin_mappings))
    {}

private:
    ProjectConfig() = default;

    static std::expected<std::unordered_map<std::string, DepType>, std::string> ParseDeps(
        const nlohmann::json::iterator& it, const std::filesystem::path& project_root);
    static std::string FormatOSPluginFilename(const std::filesystem::path& path);
};
