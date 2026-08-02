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

    try
    {
        app.parse(argc, argv);
    }
    catch(const CLI::ParseError& e)
    {
        app.exit(e);
        return;
    }

    if(!run_cmd) return;

    auto project_result = Project::Init(run_path);
    if(!project_result)
    {
        std::println(std::cerr, "{}", project_result.error());
        return;
    }

    const std::unique_ptr<Project> project = std::move(project_result.value());
    if (auto run_result = project->CompileAndRun(); !run_result)
    {
        std::println(std::cerr, "Execution Failed: {}", run_result.error());
    }

}
int main(const int argc, char** argv)
{
    Run(argc, argv);
    return 0;
}
