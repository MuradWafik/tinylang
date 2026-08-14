#include <iostream>
#include <print>

#include "CLI/CLI.hpp"
#include "project/Project.h"


void PopulateMainTL(const std::filesystem::path& target_dir, const std::string& project_name)
{
    std::ofstream out_file(target_dir / std::format("{}.tl", project_name));
    const auto contents = std::format(R"MAIN(module {};
import std;

fn main() -> void {{
    std.Println("Hello World!");
}}
)MAIN", project_name);

    out_file << contents;
    out_file.close();
}

void PopulateJsonConfig(const std::filesystem::path& path, const std::string& project_name)
{
    const nlohmann::json root =
    {
        {"name", project_name},
        {"version", "0.1.0"},
        {"entry", std::format("{}.tl", project_name)}
    };

    std::ofstream out_file(path);
    out_file << root.dump(4);
    out_file.close();
}

std::expected<void, std::string> GenerateProject(const std::filesystem::path& target_dir, const std::string& name)
{
    namespace fs = std::filesystem;

    if(fs::exists(target_dir))
    {
        if(!fs::is_directory(target_dir))
        {
            return std::unexpected(std::format("'{}' is not a valid directory", target_dir.string()));
        }

        if(fs::exists(target_dir / "tinylang.json"))
        {
            return std::unexpected(std::format("'tinylang.json' already exists in '{}'", target_dir.string()));
        }

        if(const auto entry_file = target_dir / std::format("{}.tl", name);
            fs::exists(entry_file))
        {
            return std::unexpected(std::format("'{}' already exists in '{}'", entry_file.filename().string(), target_dir.string()));
        }
    }
    else
    {
        std::error_code ec;
        fs::create_directories(target_dir, ec);
        if(ec)
        {
            return std::unexpected(std::format("Failed to create directory '{}': {}", target_dir.string(), ec.message()));
        }
    }

    PopulateMainTL(target_dir, name);
    PopulateJsonConfig(target_dir / "tinylang.json", name);
    return {};
}

void NewProject(const std::string& project_name)
{
    namespace fs = std::filesystem;
    fs::path target_dir;
    std::string name;

    if(project_name.empty() || project_name == ".")
    {
        target_dir = fs::current_path();
        name = target_dir.filename().string();
    }
    else
    {
        target_dir = fs::current_path() / project_name;
        name = fs::path(project_name).filename().string();
    }

    if(auto res = GenerateProject(target_dir, name);
        !res)
    {
        std::println(std::cerr, "Error: {}", res.error());
        return;
    }

    std::println("Created new Tinylang project '{}' in {}", name, target_dir.string());
    if(target_dir == fs::current_path())
    {
        std::println("Run 'tinylang run .' to start");
    }
    else
    {
        std::println("Run 'cd {} && tinylang run .' to start", name);
    }
}

void LintProject(const std::string& check_path, const bool output_json)
{
    auto project_result = Project::Init(check_path);
    if(!project_result)
    {
        if(output_json)
        {
            using nlohmann::json;
            auto root = json::array({
                {
                    {"file", check_path},
                    {"line", 1},
                    {"column", 1},
                    {"severity", "error"},
                    {"message", project_result.error()}
                }
            });
            fputs(root.dump().c_str(), stdout);
            fputc('\n', stdout);
        }
        else
        {
            std::println(std::cerr, "{}", project_result.error());
        }
        return;
    }

    const std::unique_ptr<Project> project = std::move(project_result.value());
    auto check_result = project->Check();
    if(!check_result)
    {
        if(output_json)
        {
            using nlohmann::json;
            auto root = json::array({
                {
                    {"file", check_path},
                    {"line", 1},
                    {"column", 1},
                    {"severity", "error"},
                    {"message", check_result.error()}
                }
            });
            fputs(root.dump().c_str(), stdout);
            fputc('\n', stdout);
        }
        else
        {
            std::println(std::cerr, "Check Failed: {}", check_result.error());
        }
        return;
    }

    const auto& diagnostics = check_result.value();
    if(!diagnostics.empty())
    {
        if(output_json)
        {
            using nlohmann::json;
            auto root = json::array(); // array of objects
            for(const auto& [message, location] : diagnostics)
            {
                root.push_back(
                    {
                        {"file", location.filename.empty() ? check_path : location.filename},
                        {"line", location.line_number == 0 ? 1 : location.line_number},
                        {"column", location.column == 0 ? 1 : location.column},
                        {"severity", "error"},
                        {"message", message}
                    }
                );
            }
            fputs(root.dump().c_str(), stdout);
            fputc('\n', stdout);
        }
        else
        {
            for(const auto& [message, location] : diagnostics)
            {
                if(location.line_number > 0)
                {
                    std::println(std::cerr, "Error: {} at {}", message, location);
                }
                else
                {
                    std::println(std::cerr, "Error: {}", message);
                }
            }
        }
    }
    else
    {
        if(output_json)
        {
            std::println("[]");
        }
        else
        {
            std::println("No issues found.");
        }
    }
}

void RunProject(const std::string& run_path)
{
    constexpr auto error = "Execution Failed";
    if(run_path.ends_with(".tlc"))
    {
        if(auto run_result = Project::RunSerialized(run_path); !run_result)
        {
            std::println(std::cerr, "{}: {}", error, run_result.error());
        }
        return;
    }

    auto project_result = Project::Init(run_path);
    if(!project_result)
    {
        std::println(std::cerr, "{}", project_result.error());
        return;
    }

    const std::unique_ptr<Project> project = std::move(project_result.value());
    if(auto run_result = project->CompileAndRun();
        !run_result)
    {
        std::println(std::cerr, "{}: {}", error, run_result.error());
    }
}

bool BuildProject(const std::string& build_path, const std::string& output_path)
{
    auto project_result = Project::Init(build_path);
    if(!project_result)
    {
        std::println(std::cerr, "{}", project_result.error());
        return false;
    }

    const std::unique_ptr<Project> project = std::move(project_result.value());
    if(auto build_result = project->CompileOnly(output_path);
        !build_result)
    {
        std::println(std::cerr, "Build Failed: {}", build_result.error());
        return false;
    }

    return true;
}

void ListSymbols(const std::string& target_path, const bool output_json)
{
    auto project_result = Project::Init(target_path);
    if(!project_result)
    {
        if(output_json)
        {
            std::println("[]");
        }
        else
        {
            std::println(std::cerr, "Failed to initialize project for symbols: {}", project_result.error());
        }
        return;
    }

    const std::unique_ptr<Project> project = std::move(project_result.value());
    auto symbols_result = project->ExtractSymbols();
    if(!symbols_result)
    {
        if(output_json)
        {
            std::println("[]");
        }
        else
        {
            std::println(std::cerr, "Failed to extract symbols: {}", symbols_result.error());
        }
        return;
    }

    const auto& symbols = symbols_result.value();
    if(output_json)
    {
        using nlohmann::json;
        auto root = json::array();
        for(const auto& sym : symbols)
        {
            root.push_back({
                {"name", sym.name},
                {"kind", sym.kind},
                {"detail", sym.detail},
                {"file", sym.location.filename},
                {"line", sym.location.line_number},
                {"column", sym.location.column},
                {"container", sym.container_name},
                {"exported", sym.is_exported}
            });
        }
        fputs(root.dump(2).c_str(), stdout);
        fputc('\n', stdout);
    }
    else
    {
        for(const auto& sym : symbols)
        {
            if(!sym.container_name.empty())
            {
                std::println("[{}] {}.{} {} ({}:{}:{})",
                    sym.kind, sym.container_name, sym.name, sym.detail,
                    sym.location.filename, sym.location.line_number, sym.location.column);
            }
            else
            {
                std::println("[{}] {} {} ({}:{}:{})",
                    sym.kind, sym.name, sym.detail,
                    sym.location.filename, sym.location.line_number, sym.location.column);
            }
        }
    }
}

void ShowProjectInfo(const std::string& target_path, const bool output_json)
{
    auto project_result = Project::Init(target_path);
    if(!project_result)
    {
        if(output_json)
        {
            std::println("{{}}");
        }
        else
        {
            std::println(std::cerr, "Failed to load project: {}", project_result.error());
        }
        return;
    }

    const auto info = project_result.value()->GetInfo();
    if(output_json)
    {
        using nlohmann::json;
        json root = {
            {"name", info.name},
            {"version", info.version},
            {"project_root", info.project_root},
            {"entry_point", info.entry_point},
            {"source_files", info.source_files},
            {"std_path", info.std_path}
        };
        fputs(root.dump(2).c_str(), stdout);
        fputc('\n', stdout);
    }
    else
    {
        std::println("Project: {} (v{})", info.name, info.version);
        std::println("Root: {}", info.project_root);
        std::println("Entry: {}", info.entry_point);
        std::println("StdLib: {}", info.std_path);
        std::println("Sources ({}):", info.source_files.size());
        for(const auto& s : info.source_files)
        {
            std::println("  - {}", s);
        }
    }
}

void FindDefinitionCmd(const std::string& project_path, const std::string& file_path, uint32_t line, uint32_t col, const bool output_json)
{
    auto project_result = Project::Init(project_path);
    if(!project_result)
    {
        if(output_json)
        {
            std::println("{{}}");
        }
        else
        {
            std::println(std::cerr, "Failed to load project: {}", project_result.error());
        }
        return;
    }

    auto def_result = project_result.value()->FindDefinition(file_path, line, col);
    if(!def_result)
    {
        if(output_json)
        {
            std::println("{{}}");
        }
        else
        {
            std::println(std::cerr, "{}", def_result.error());
        }
        return;
    }

    const auto& def = def_result.value();
    if(output_json)
    {
        using nlohmann::json;
        json root = {
            {"name", def.name},
            {"kind", def.kind},
            {"file", def.location.filename},
            {"line", def.location.line_number},
            {"column", def.location.column}
        };
        fputs(root.dump(2).c_str(), stdout);
        fputc('\n', stdout);
    }
    else
    {
        std::println("{} ({}) defined at {}:{}:{}", def.name, def.kind, def.location.filename, def.location.line_number, def.location.column);
    }
}

int Run(const int argc, char** argv)
{
    CLI::App app{"Tinylang Compiler"};

    std::string project_name = ".";
    const auto new_cmd = app.add_subcommand("new", "Create a new Tinylang project");
    new_cmd->add_option("name", project_name, "Name of the project")->required();


    std::string check_path = ".";
    const auto check_cmd = app.add_subcommand("check", "Lint a Tinylang file");
    check_cmd->add_option("path", check_path, "Path to TinyLang file");

    bool output_json = false;
    check_cmd->add_flag("--json", output_json, "Output the linting in json format");

    std::string symbols_path = ".";
    const auto symbols_cmd = app.add_subcommand("symbols", "Extract document/workspace symbols");
    symbols_cmd->add_option("path", symbols_path, "Path to project root or file");
    bool symbols_json = false;
    symbols_cmd->add_flag("--json", symbols_json, "Output symbols in JSON format");

    std::string info_path = ".";
    const auto info_cmd = app.add_subcommand("info", "Show project configuration and discovered source files");
    info_cmd->add_option("path", info_path, "Path to project root");
    bool info_json = false;
    info_cmd->add_flag("--json", info_json, "Output info in JSON format");

    std::string def_proj = ".";
    std::string def_file;
    uint32_t def_line = 1;
    uint32_t def_col = 1;
    bool def_json = false;
    const auto def_cmd = app.add_subcommand("definition", "Find definition of symbol at line:column");
    def_cmd->add_option("file", def_file, "Source file path")->required();
    def_cmd->add_option("line", def_line, "Line number (1-based)")->required();
    def_cmd->add_option("col", def_col, "Column number (1-based)")->required();
    def_cmd->add_option("-p,--project", def_proj, "Path to project root");
    def_cmd->add_flag("--json", def_json, "Output definition in JSON format");

    std::string run_path = ".";
    const auto run_cmd = app.add_subcommand("run", "Run a Tinylang project");
    run_cmd->add_option("path", run_path, "Path to project root");


    std::string build_path = ".";
    std::string output_path = ".";
    const auto build_cmd = app.add_subcommand("build", "Build a Tinylang project");
    build_cmd->add_option("path", build_path, "Path to project root");
    build_cmd->add_option("-o,--output", output_path, "Output directory or file path");

    try
    {
        app.parse(argc, argv);
    }
    catch(const CLI::ParseError& e)
    {
        return app.exit(e);
    }

    if(run_cmd->parsed())
    {
        RunProject(run_path);
    }
    else if(build_cmd->parsed())
    {
        if(!BuildProject(build_path, output_path))
        {
            return 1;
        }
    }
    else if(check_cmd->parsed())
    {
        LintProject(check_path, output_json);
    }
    else if(symbols_cmd->parsed())
    {
        ListSymbols(symbols_path, symbols_json);
    }
    else if(info_cmd->parsed())
    {
        ShowProjectInfo(info_path, info_json);
    }
    else if(def_cmd->parsed())
    {
        FindDefinitionCmd(def_proj, def_file, def_line, def_col, def_json);
    }
    else if(new_cmd->parsed())
    {
        NewProject(project_name);
    }

    return 0;
}
int main(const int argc, char** argv)
{
    return Run(argc, argv);
}
