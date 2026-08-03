#include "project/Project.h"
#include <functional>
#include <unordered_set>

#include "analysis/SemanticAnalyzer.h"
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "project/ModuleRegistry.h"
#include "vm/Compiler.h"
#include "vm/Serializer.h"
#include "vm/VM.h"

std::expected<std::unique_ptr<Project>, std::string> Project::Init(const std::filesystem::path& path)
{
    auto project_config = ProjectConfig::FindAndLoad(path);
    if(!project_config) return std::unexpected(project_config.error());
    if(!project_config.value()->entry_point)
    {
        return std::unexpected("no entry point");
    }

    std::vector<std::filesystem::path> tl_files;
    namespace fs = std::filesystem;
    auto collect_files = [&](const std::filesystem::path& root_dir)
    {
        for(auto it = fs::recursive_directory_iterator(root_dir);
            it != fs::recursive_directory_iterator(); ++it)
        {
            const auto& entry = *it;

            if(entry.is_directory() && entry.path().filename().string().starts_with("."))
            {
                it.disable_recursion_pending();
                continue;
            }

            if(entry.is_directory() && fs::exists(entry.path() / "tinylang.json"))
            {
                if (entry.path() != root_dir) {
                    it.disable_recursion_pending();
                    continue;
                }
            }

            if(entry.is_regular_file() && entry.path().extension() == ".tl")
            {
                tl_files.push_back(entry.path());
            }
        }
    };

    collect_files(project_config.value()->project_root);
    
    // Also collect files from dependencies
    for(const auto& dep_variant: project_config.value()->dependencies | std::views::values)
    {
        if (std::holds_alternative<ProjectConfig::LocalDependency>(dep_variant))
        {
            const auto& [path] = std::get<ProjectConfig::LocalDependency>(dep_variant);
            auto dep_path = path.parent_path();
            collect_files(dep_path);
        }
    }
    return std::unique_ptr<Project>(new Project(std::move(tl_files), std::move(project_config.value())));
}

std::expected<void, std::string> Project::CompileAndRun()
{
    for (const auto& file_path : tl_files)
    {
        auto file_open_result = FileReader::Read(file_path);
        if(!file_open_result) return std::unexpected(file_open_result.error());

        Lexer lexer{};
        auto lex_result = lexer.Lex(file_open_result.value(), file_path.string());
        if(!lex_result)
        {
            return std::unexpected(std::format(
                "Error Lexing: {} at {}", lex_result.error().message, lex_result.error().location)
            );
        }

        Parser parser{lex_result.value()};
        auto parse_result = parser.ParseProgram(); // ast for this file
        if(!parse_result)
        {
            return std::unexpected(parse_result.error());
        }

        std::string module_name = parser.module_name;
        if(module_name.empty()) return std::unexpected("No module name found");

        module_registry->RegisterModule(module_name, std::move(parse_result.value()));
    }

    SemanticAnalyzer semantic_analyzer{project_config.get(), module_registry.get()};

    // The analyzer will do Pass 1 (find exports) and Pass 2 (typecheck) internally
    if (auto analysis_result = semantic_analyzer.AnalyzeAll(); !analysis_result)
    {
        return std::unexpected(analysis_result.error());
    }

    Compiler compiler{project_config.get(), module_registry.get()};
    const auto chunks = compiler.CompileAll("main");
    
    std::vector<std::string> ordered_modules;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> in_path;

    std::function<std::expected<void, std::string>(const std::string&)> visit = [&](const std::string& module_name) -> std::expected<void, std::string>
    {
        if(in_path.contains(module_name))
        {
            return std::unexpected(std::format("Circular dependency detected involving module '{}'", module_name));
        }

        if(visited.contains(module_name)) return {};


        in_path.insert(module_name);
        if(const auto* ns = module_registry->GetNamespace(module_name))
        {
            for(const auto& dep : ns->dependencies)
            {
                if(auto res = visit(dep); !res) return res;
            }
        }
        in_path.erase(module_name);
        visited.insert(module_name);
        ordered_modules.push_back(module_name);
        return {};
    };

    if(auto res = visit("main"); !res) return std::unexpected(res.error());

    VM vm;
    if(project_config)
    {
        vm.SetProjectRoot(project_config->project_root);
    }

    if(auto res = vm.StartProgram(chunks, ordered_modules); res != InterpretResult::INTERPRET_OK)
    {
        return std::unexpected(std::format("VM exited with error code: {}", static_cast<int>(res)));
    }

    return {};
}

std::expected<void, std::string> Project::CompileOnly(const std::string& output_path) const
{
    for(const auto& file_path : tl_files)
    {
        auto file_open_result = FileReader::Read(file_path);
        if(!file_open_result) return std::unexpected(file_open_result.error());

        Lexer lexer{};
        auto lex_result = lexer.Lex(file_open_result.value(), file_path.string());
        if(!lex_result)
        {
            return std::unexpected(std::format(
                "Error Lexing: {} at {}", lex_result.error().message, lex_result.error().location)
            );
        }

        Parser parser{lex_result.value()};
        auto parse_result = parser.ParseProgram();
        if(!parse_result)
        {
            return std::unexpected(parse_result.error());
        }

        std::string module_name = parser.module_name;
        if(module_name.empty()) return std::unexpected("No module name found");

        module_registry->RegisterModule(module_name, std::move(parse_result.value()));
    }

    SemanticAnalyzer semantic_analyzer{project_config.get(), module_registry.get()};
    if(auto analysis_result = semantic_analyzer.AnalyzeAll(); !analysis_result)
    {
        return std::unexpected(analysis_result.error());
    }

    Compiler compiler{project_config.get(), module_registry.get()};
    const auto chunks = compiler.CompileAll("main");
    
    std::vector<std::string> ordered_modules;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> in_path;

    std::function<std::expected<void, std::string>(const std::string&)> visit = [&](const std::string& module_name) -> std::expected<void, std::string>
    {
        if(in_path.contains(module_name))
        {
            return std::unexpected(std::format("Circular dependency detected involving module '{}'", module_name));
        }

        if(visited.contains(module_name)) return {};

        in_path.insert(module_name);
        if(const auto* ns = module_registry->GetNamespace(module_name))
        {
            for(const auto& dep : ns->dependencies)
            {
                if(auto res = visit(dep); !res) return res;
            }
        }
        in_path.erase(module_name);
        visited.insert(module_name);
        ordered_modules.push_back(module_name);
        return {};
    };

    if(auto res = visit("main"); !res)
    {
        return std::unexpected(res.error());
    }

    return Serializer::Serialize(chunks, ordered_modules, output_path);
}

std::expected<void, std::string> Project::RunSerialized(const std::string& input_path)
{
    auto deserialize_result = Serializer::Deserialize(input_path);
    if(!deserialize_result) return std::unexpected(deserialize_result.error());

    auto [chunks, ordered_modules] = std::move(deserialize_result.value());
    VM vm;
    vm.SetProjectRoot(std::filesystem::path(input_path).parent_path());
    if(auto res = vm.StartProgram(chunks, ordered_modules); res != InterpretResult::INTERPRET_OK)
    {
        return std::unexpected(std::format("VM exited with error code: {}", static_cast<int>(res)));
    }

    return {};
}
