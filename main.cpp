#include <iostream>
#include <print>

#include "FileReader.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "TreeWalkInterpreter.h"


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

    TreeWalkInterpreter tree_walk_interpreter{semantic_analyzer};
    for(const auto& node: parse_result.value())
    {
        if(auto* stmt = dynamic_cast<Statement*>(node.get())) tree_walk_interpreter.Execute(stmt);
        else if(auto* expr = dynamic_cast<Expression*>(node.get())) tree_walk_interpreter.Evaluate(expr);
    }

}
int main(const int argc, char** argv)
{
    Run(argc, argv);
    return 0;
}
