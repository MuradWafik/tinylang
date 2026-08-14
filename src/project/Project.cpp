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
    
    for(const auto& dep_variant: project_config.value()->dependencies | std::views::values)
    {
        if (std::holds_alternative<ProjectConfig::LocalDependency>(dep_variant))
        {
            const auto& [path] = std::get<ProjectConfig::LocalDependency>(dep_variant);
            auto dep_path = path.parent_path();
            collect_files(dep_path);
        }
    }

    const auto std_path = GetBundledStdPath();
    if (fs::exists(std_path) && std_path != project_config.value()->project_root && !std_path.string().starts_with(project_config.value()->project_root.string()))
    {
        collect_files(std_path);
    }
    return std::unique_ptr<Project>(new Project(std::move(tl_files), std::move(project_config.value())));
}

std::expected<void, std::string> Project::CompileAndRun()
{
    std::string entry_module_name;
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
        auto parse_result = parser.ParseProgram();
        if(!parse_result)
        {
            return std::unexpected(parse_result.error());
        }

        std::string module_name = parser.module_name;
        if(module_name.empty()) return std::unexpected("No module name found");

        if(project_config)
        {
            std::error_code ec;
            if(project_config->entry_point && std::filesystem::equivalent(file_path, project_config->project_root / project_config->entry_point.value(), ec))
            {
                entry_module_name = module_name;
            }
        }
        else if(entry_module_name.empty())
        {
            entry_module_name = module_name;
        }

        module_registry->RegisterModule(module_name, std::move(parse_result.value()));
    }
    if(entry_module_name.empty()) entry_module_name = "main";

    SemanticAnalyzer semantic_analyzer{project_config.get(), module_registry.get(), true, entry_module_name};

    if (auto analysis_result = semantic_analyzer.AnalyzeAll(); !analysis_result)
    {
        return std::unexpected(analysis_result.error());
    }

    Compiler compiler{project_config.get(), module_registry.get()};
    const auto chunks = compiler.CompileAll(entry_module_name);
    
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

    if(auto res = visit(entry_module_name); !res) return std::unexpected(res.error());

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
    std::string entry_module_name;
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
        auto parse_result = parser.ParseProgram();
        if(!parse_result)
        {
            return std::unexpected(parse_result.error());
        }

        std::string module_name = parser.module_name;
        if(module_name.empty()) return std::unexpected("No module name found");

        if(project_config)
        {
            std::error_code ec;
            if(project_config->entry_point && std::filesystem::equivalent(file_path, project_config->project_root / project_config->entry_point.value(), ec))
            {
                entry_module_name = module_name;
            }
        }
        else if(entry_module_name.empty())
        {
            entry_module_name = module_name;
        }

        module_registry->RegisterModule(module_name, std::move(parse_result.value()));
    }
    if(entry_module_name.empty()) entry_module_name = "main";

    SemanticAnalyzer semantic_analyzer{project_config.get(), module_registry.get(), true, entry_module_name};
    if(auto analysis_result = semantic_analyzer.AnalyzeAll(); !analysis_result)
    {
        return std::unexpected(analysis_result.error());
    }

    Compiler compiler{project_config.get(), module_registry.get()};
    const auto chunks = compiler.CompileAll(entry_module_name);
    
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

    if(auto res = visit(entry_module_name); !res)
    {
        return std::unexpected(res.error());
    }

    std::string default_name;
    if(project_config && project_config->entry_point)
    {
        default_name = project_config->entry_point->stem().string() + ".tlc";
    }
    else
    {
        default_name = entry_module_name + ".tlc";
    }

    std::filesystem::path final_output_path;
    if(output_path.empty())
    {
        final_output_path = (project_config ? project_config->project_root : std::filesystem::current_path()) / default_name;
    }
    else
    {
        std::filesystem::path out_p(output_path);
        if(std::filesystem::is_directory(out_p) || out_p.extension() != ".tlc")
        {
            final_output_path = out_p / default_name;
        }
        else
        {
            final_output_path = out_p;
        }
    }

    return Serializer::Serialize(chunks, ordered_modules, final_output_path.string());
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

std::expected<std::vector<Diagnostic>, std::string> Project::Check() const
{
    std::string entry_module_name;
    std::vector<Diagnostic> diagnostics;

    for(const auto& file_path : tl_files)
    {
        auto file_open_result = FileReader::Read(file_path);
        if(!file_open_result)
        {
            diagnostics.push_back(Diagnostic{file_open_result.error(), SourceLocation{file_path.string(), 1, 1}});
            continue;
        }

        Lexer lexer{};
        auto lex_result = lexer.Lex(file_open_result.value(), file_path.string());
        if(!lex_result)
        {
            diagnostics.push_back(Diagnostic{lex_result.error().message, lex_result.error().location});
            continue;
        }

        Parser parser{lex_result.value()};
        auto parse_result = parser.ParseProgram();
        if(!parse_result)
        {
            diagnostics.push_back(Diagnostic::FromError(parse_result.error(), file_path.string()));
            continue;
        }

        std::string module_name = parser.module_name;
        if(module_name.empty())
        {
            diagnostics.push_back(Diagnostic{"No module name found", SourceLocation{file_path.string(), 1, 1}});
            continue;
        }

        if(project_config)
        {
            std::error_code ec;
            if(project_config->entry_point && std::filesystem::equivalent(file_path, project_config->project_root / project_config->entry_point.value(), ec))
            {
                entry_module_name = module_name;
            }
        }
        else if(entry_module_name.empty())
        {
            entry_module_name = module_name;
        }

        module_registry->RegisterModule(module_name, std::move(parse_result.value()));
    }
    if(entry_module_name.empty()) entry_module_name = "main";

    SemanticAnalyzer semantic_analyzer{project_config.get(), module_registry.get(), false, entry_module_name};
    auto analysis_result = semantic_analyzer.AnalyzeAll();

    for(const auto& diag : semantic_analyzer.GetDiagnostics())
    {
        diagnostics.push_back(diag);
    }

    if(!analysis_result && diagnostics.empty())
    {
        diagnostics.push_back(Diagnostic::FromError(analysis_result.error(), entry_module_name));
    }

    return diagnostics;
}

std::vector<DocumentSymbol> Project::ExtractSymbolsFromAST(const std::vector<std::unique_ptr<ASTNode>>& asts)
{
    std::vector<DocumentSymbol> symbols;
    for(const auto& node : asts)
    {
        if(!node) continue;
        if(const auto* fn = dynamic_cast<const FunctionDeclaration*>(node.get()))
        {
            std::string params;
            for(size_t i = 0; i < fn->method_signature.parameters.size(); ++i)
            {
                params += fn->method_signature.parameters[i].name.lexeme + ": " + fn->method_signature.parameters[i].type_name.lexeme;
                if(i + 1 < fn->method_signature.parameters.size()) params += ", ";
            }
            std::string ret = fn->method_signature.return_type ? fn->method_signature.return_type->lexeme : "void";
            std::string detail = "(" + params + ") -> " + ret;
            std::string kind = fn->receiver.has_value() ? "method" : "function";
            std::string container = fn->receiver.has_value() ? fn->receiver->type_name.lexeme : "";

            symbols.push_back({
                fn->method_signature.name.lexeme,
                kind,
                detail,
                fn->method_signature.name.source_location,
                container,
                fn->is_exported
            });
        }
        else if(const auto* st = dynamic_cast<const StructDeclaration*>(node.get()))
        {
            std::string fields;
            for(size_t i = 0; i < st->fields.size(); ++i)
            {
                fields += st->fields[i].first.lexeme + ": " + st->fields[i].second.lexeme;
                if(i + 1 < st->fields.size()) fields += ", ";
            }
            symbols.push_back({
                st->name.lexeme,
                "struct",
                "{" + fields + "}",
                st->name.source_location,
                "",
                st->is_exported
            });
        }
        else if(const auto* en = dynamic_cast<const EnumDeclaration*>(node.get()))
        {
            symbols.push_back({
                en->name.lexeme,
                "enum",
                "",
                en->name.source_location,
                "",
                en->is_exported
            });
        }
        else if(const auto* itf = dynamic_cast<const InterfaceDeclaration*>(node.get()))
        {
            symbols.push_back({
                itf->name.lexeme,
                "interface",
                "",
                itf->name.source_location,
                "",
                itf->is_exported
            });
        }
        else if(const auto* var = dynamic_cast<const VariableDeclaration*>(node.get()))
        {
            std::string type_str = var->type ? var->type->lexeme : "inferred";
            symbols.push_back({
                var->name.lexeme,
                "variable",
                type_str,
                var->name.source_location,
                "",
                var->is_exported
            });
        }
        else if(const auto* native_fn = dynamic_cast<const NativeFunctionDeclaration*>(node.get()))
        {
            std::string params;
            for(size_t i = 0; i < native_fn->method_signature.parameters.size(); ++i)
            {
                params += native_fn->method_signature.parameters[i].name.lexeme + ": " + native_fn->method_signature.parameters[i].type_name.lexeme;
                if(i + 1 < native_fn->method_signature.parameters.size()) params += ", ";
            }
            std::string ret = native_fn->method_signature.return_type ? native_fn->method_signature.return_type->lexeme : "void";
            std::string detail = "native (" + params + ") -> " + ret;

            symbols.push_back({
                native_fn->method_signature.name.lexeme,
                "native_function",
                detail,
                native_fn->method_signature.name.source_location,
                "",
                false
            });
        }
    }
    return symbols;
}

std::expected<std::vector<DocumentSymbol>, std::string> Project::ExtractSymbols() const
{
    std::vector<DocumentSymbol> symbols;
    for(const auto& file_path : tl_files)
    {
        auto file_open_result = FileReader::Read(file_path);
        if(!file_open_result) continue;

        Lexer lexer{};
        auto lex_result = lexer.Lex(file_open_result.value(), file_path.string());
        if(!lex_result) continue;

        Parser parser{lex_result.value()};
        auto parse_result = parser.ParseProgram();
        if(!parse_result) continue;

        auto file_symbols = ExtractSymbolsFromAST(parse_result.value());
        symbols.insert(symbols.end(), file_symbols.begin(), file_symbols.end());
    }
    return symbols;
}

ProjectInfo Project::GetInfo() const
{
    ProjectInfo info;
    if(project_config)
    {
        info.name = project_config->name;
        info.version = project_config->version;
        info.project_root = project_config->project_root.string();
        info.entry_point = project_config->entry_point ? project_config->entry_point.value().string() : "";
    }
    for(const auto& f : tl_files)
    {
        info.source_files.push_back(f.string());
    }
    info.std_path = GetBundledStdPath().string();
    return info;
}

std::expected<DefinitionResult, std::string> Project::FindDefinition(const std::string& file_path, uint32_t line, uint32_t col) const
{
    auto file_open_result = FileReader::Read(file_path);
    if(!file_open_result)
    {
        return std::unexpected(file_open_result.error());
    }

    Lexer lexer{};
    auto lex_result = lexer.Lex(file_open_result.value(), file_path);
    if(!lex_result)
    {
        return std::unexpected("Failed to tokenize source file");
    }

    std::string target_identifier;
    for(const auto& tok : lex_result.value())
    {
        if(tok.source_location.line_number == line)
        {
            uint32_t start_col = tok.source_location.column;
            uint32_t end_col = start_col + static_cast<uint32_t>(tok.lexeme.length());
            if(col >= start_col && col <= end_col)
            {
                if(tok.type == TokenType::Identifier || tok.IsPrimitiveTypeName())
                {
                    target_identifier = tok.lexeme;
                    break;
                }
            }
        }
    }

    if(target_identifier.empty())
    {
        return std::unexpected("No identifier found at given position");
    }

    auto all_symbols_res = ExtractSymbols();
    if(!all_symbols_res)
    {
        return std::unexpected(all_symbols_res.error());
    }

    const auto& symbols = all_symbols_res.value();
    
    // First, search for exact match in current file
    for(const auto& sym : symbols)
    {
        if(sym.name == target_identifier && sym.location.filename == file_path)
        {
            return DefinitionResult{sym.name, sym.kind, sym.location};
        }
    }

    // Next, search in any project file
    for(const auto& sym : symbols)
    {
        if(sym.name == target_identifier)
        {
            return DefinitionResult{sym.name, sym.kind, sym.location};
        }
    }

    return std::unexpected(std::format("Definition for '{}' not found", target_identifier));
}
