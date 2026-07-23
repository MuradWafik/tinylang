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

    auto FileOpenResult = FileReader::Read(argv[1]);
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

    SemanticAnalyzer semantic_analyzer{};
    if(auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value()); !semantic_analysis_result)
    {
        std::println(std::cerr, "Error in semantic analysis: {}", semantic_analysis_result.error());
        return;
    }

    Compiler compiler;
    std::unique_ptr<Chunk> chunk = compiler.Compile(parse_result.value());

    // 4. Disassemble the compiled chunk to verify it worked
    chunk->Disassemble("Compiler Test");

    // 5. Run it in the VM!
    VM vm;
    vm.Interpret(chunk.get());

}
int main(const int argc, char** argv)
{
    Run(argc, argv);
    return 0;
}
