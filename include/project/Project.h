#pragma once
#include "project/ModuleRegistry.h"
#include "project/ProjectConfig.h"

using path = std::filesystem::path;

class Project
{
public:
    static std::expected<std::unique_ptr<Project>, std::string> Init(const std::filesystem::path& path);

    
    std::expected<void, std::string> CompileAndRun();
    std::expected<void, std::string> CompileOnly(const std::string& output_path) const;
    static std::expected<void, std::string> RunSerialized(const std::string& input_path);

private:
    Project(std::vector<path> tl_files, std::unique_ptr<ProjectConfig> project_config)
    : tl_files{std::move(tl_files)}, project_config{std::move(project_config)}, module_registry{std::make_unique<ModuleRegistry>()}
    {}

    std::vector<path> tl_files;
    std::unique_ptr<ProjectConfig> project_config;
    std::unique_ptr<ModuleRegistry> module_registry;
};
