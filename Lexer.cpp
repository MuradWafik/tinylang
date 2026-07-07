#include "Lexer.h"

#include <cctype>
#include <print>


bool Lexer::IsAtEnd() const
{
    return char_index == source.size() - 2;
}

void Lexer::SkipWhitespace()
{
    while(source.at(char_index) == ' ')
    {
        char_index++;
    }
}

Token Lexer::LexIdentifier()
{
    char_index++;
    return {};
}

Token Lexer::LexNumber()
{
    std::string number_string = "";

    char c = source.at(char_index);
    while(std::isdigit(c))
    {
        number_string += c;
        c = source.at(++char_index);
    }
    return Token{TokenType::Int, number_string};
}

Token Lexer::LexString()
{
    char_index++;
    return {};
}

Token Lexer::LexSymbol()
{
    char_index++;
    return {};
}

char Lexer::Peek() const
{
    return source.at(char_index);
}


std::expected<std::vector<Token>, std::string> Lexer::Lex(const std::string_view source)
{
    this->source = source;

    std::vector<Token> tokens;
    while (!IsAtEnd())
    {

        SkipWhitespace();
        if (IsAtEnd())
        {
            break;
        }

        const char cur = Peek();
        std::println("Current Char: {}", cur);
        if(std::isalpha((cur)) || cur == '_')
        {
            tokens.push_back(LexIdentifier());
        }
        else if(std::isdigit(cur))
        {
            auto e = LexNumber();
            std::println("Got number {}", e.lexeme);
            tokens.push_back(LexNumber());
        }
        else if(cur == '"')
        {
            tokens.push_back(LexString());
        }
        else
        {
            tokens.push_back(LexSymbol());
        }
    }

    tokens.push_back(Token(TokenType::EndOfFile));

    return {};
}
