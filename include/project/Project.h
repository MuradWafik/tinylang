#pragma once
#include "analysis/SemanticAnalyzer.h"
#include "project/ModuleRegistry.h"
#include "project/ProjectConfig.h"

using path = std::filesystem::path;

struct DocumentSymbol
{
    std::string name;
    std::string kind; // "function", "method", "struct", "enum", "interface", "variable", "native_function"
    std::string detail;
    SourceLocation location;
    std::string container_name;
    bool is_exported = false;
};

struct ProjectInfo
{
    std::string name;
    std::string version;
    std::string project_root;
    std::string entry_point;
    std::vector<std::string> source_files;
    std::string std_path;
};

struct DefinitionResult
{
    std::string name;
    std::string kind;
    SourceLocation location;
};

class Project
{
public:
    static std::expected<std::unique_ptr<Project>, std::string> Init(const std::filesystem::path& path);

    
    std::expected<void, std::string> CompileAndRun();
    std::expected<void, std::string> CompileOnly(const std::string& output_path) const;
    std::expected<std::vector<Diagnostic>, std::string> Check() const;
    std::expected<std::vector<DocumentSymbol>, std::string> ExtractSymbols() const;
    static std::vector<DocumentSymbol> ExtractSymbolsFromAST(const std::vector<std::unique_ptr<ASTNode>>& asts);
    ProjectInfo GetInfo() const;
    std::expected<DefinitionResult, std::string> FindDefinition(const std::string& file_path, uint32_t line, uint32_t col) const;
    static std::expected<void, std::string> RunSerialized(const std::string& input_path);

private:
    Project(std::vector<path> tl_files, std::unique_ptr<ProjectConfig> project_config)
    : tl_files{std::move(tl_files)}, project_config{std::move(project_config)}, module_registry{std::make_unique<ModuleRegistry>()}
    {}

    std::vector<path> tl_files;
    std::unique_ptr<ProjectConfig> project_config;
    std::unique_ptr<ModuleRegistry> module_registry;
};
