#include <iostream>
#include <print>

#include "CLI/CLI.hpp"
#include "project/Project.h"



void Run(const int argc, char** argv)
{
    CLI::App app{"Tinylang Compiler"};

    // std::string project_name;
    // auto new_cmd = app.add_subcommand("new", "Create a new Tinylang project");
    // new_cmd->add_option("name", project_name, "Name of the project")->required();
    //

    std::string run_path = ".";
    const auto run_cmd = app.add_subcommand("run", "Run a Tinylang project");
    run_cmd->add_option("path", run_path, "Path to project root");

    std::string build_path = ".";
    std::string output_path = "out.tlc";
    const auto build_cmd = app.add_subcommand("build", "Build a Tinylang project");
    build_cmd->add_option("path", build_path, "Path to project root");
    build_cmd->add_option("-o,--output", output_path, "Output file path");

    try
    {
        app.parse(argc, argv);
    }
    catch(const CLI::ParseError& e)
    {
        app.exit(e);
        return;
    }

    if(run_cmd->parsed())
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
        if (auto run_result = project->CompileAndRun(); !run_result)
        {
            std::println(std::cerr, "{}: {}", error, run_result.error());
        }
    }
    else if(build_cmd->parsed())
    {
        auto project_result = Project::Init(build_path);
        if(!project_result)
        {
            std::println(std::cerr, "{}", project_result.error());
            return;
        }

        const std::unique_ptr<Project> project = std::move(project_result.value());
        if(auto build_result = project->CompileOnly(output_path); !build_result)
        {
            std::println(std::cerr, "Build Failed: {}", build_result.error());
        }
    }
}
int main(const int argc, char** argv)
{
    Run(argc, argv);
    return 0;
}
