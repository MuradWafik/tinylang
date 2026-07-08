#include <iostream>
#include <print>

#include "FileReader.h"
#include "Lexer.h"

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
        std::println(std::cerr, "Line {}, Column {}", LexResult.error().location.line_number, LexResult.error().location.column);
    }
    else
    {
        for(const auto& token: LexResult.value())
        {
            std::println("Type: {} Value: {}", Token::TypeToString(token.type), token.lexeme);
        }
    }

    return 0;
}
