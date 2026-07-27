#include <iostream>
#include <print>

#include "utils/Utils.h"
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "analysis/SemanticAnalyzer.h"

#include "vm/Compiler.h"
#include "vm/VM.h"


void Run(const int argc, char** argv)
{
    if(argc < 2)
    {
        std::println(std::cerr, "No File Path provided");
        return;
    }

    const auto source_file_path = argv[1];
    auto FileOpenResult = FileReader::Read(source_file_path);
    if(!FileOpenResult.has_value())
    {
        std::println(std::cerr, "{}", FileOpenResult.error());
        return;
    }

    Lexer lexer{};
    auto LexResult = lexer.Lex(FileOpenResult.value());
    if(!LexResult)
    {
        std::println(std::cerr, "Error Lexing: {}", LexResult.error().message);
        std::println(std::cerr, "{}", LexResult.error().location);
        return;
    }

    Parser parser{LexResult.value()};
    auto parse_result = parser.ParseProgram();

    if(!parse_result)
    {
        std::println(std::cerr, "Error Parsing: {}", parse_result.error());
        return;
    }

    auto project_config =  ProjectConfig::FindAndLoad(source_file_path);
    if(!project_config)
    {
        std::println(std::cerr, "Error reading project config: {}", project_config.error());
        return;
    }

    SemanticAnalyzer semantic_analyzer{project_config->get()};

    if(auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value()); !semantic_analysis_result)
    {
        std::println(std::cerr, "Error in semantic analysis: {}", semantic_analysis_result.error());
        return;
    }

    Compiler compiler{project_config->get()};
    std::unique_ptr<Chunk> chunk = compiler.Compile(parse_result.value());

    // chunk->Disassemble("Compiler Test");

    VM vm;
    vm.StartProgram(chunk.get());

}
int main(const int argc, char** argv)
{
    Run(argc, argv);
    return 0;
}
