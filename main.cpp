#include <iostream>
#include <print>

#include "FileReader.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        std::println(std::cerr, "No File Path provided");
    }


    auto FileOpenResult = FileReader::Read(argv[1]);
    if(!FileOpenResult.has_value())
    {
        std::println(std::cerr, "{}", FileOpenResult.error());
    }

    Lexer lexer{};
    auto LexResult = lexer.Lex(FileOpenResult.value());

    if(!LexResult)
    {
        std::println(std::cerr, "Error Lexing: {}", LexResult.error().message);
        std::println(std::cerr, "{}", LexResult.error().location);
    }
    else
    {
        for(const auto& token: LexResult.value())
        {
            std::println("Type: {} Value: {}", Token::TypeToString(token.type), token.lexeme);
        }
    }

    Parser parser{LexResult.value()};
    auto parse_result = parser.ParseProgram();

    if(!parse_result)
    {
        std::println(std::cerr, "Error Parsing: {}", parse_result.error());
    }
    else
    {
        std::println("Number of parsed tokens: {}", parse_result->size());
        for(const auto& ast_node : parse_result.value())
        {
            std::println("Parsed Token: {}", ast_node.get());
        }
    }

    SemanticAnalyzer semantic_analyzer{};
    auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value());
    if(!semantic_analysis_result)
    {
        std::println(std::cerr, "Error in semantic analysis: {}", semantic_analysis_result.error());
    }

    return 0;
}
