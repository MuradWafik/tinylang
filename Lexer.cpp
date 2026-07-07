#include "Lexer.h"

#include <cassert>
#include <cctype>
#include <print>


bool Lexer::IsAtEnd() const
{
    return char_index >= source.size();
}

void Lexer::SkipWhitespace()
{
    while(!IsAtEnd() && Peek() == ' ')
    {
        Consume();
    }
}

std::expected<Token, LexerError>  Lexer::LexIdentifier()
{
    Consume();
    return {};
}

std::expected<Token, LexerError>  Lexer::LexNumber()
{
    std::string number_string = "";
    TokenType token_type{TokenType::Int};
    char c = Peek();
    while(!IsAtEnd() && (std::isdigit(c) || c == '.'))
    {
        if(c == '.')
        {
            token_type = TokenType::Float;
        }

        number_string += c;
        c = PeekNext();
        Consume();
    }
    return Token{token_type, number_string};
}

std::expected<Token, LexerError>  Lexer::LexString()
{
    std::string lexene = "";

    char c = Peek();
    assert(c == '"');
    Consume();

    while(!IsAtEnd())
    {
        c = Peek();
        if(c == '"')
        {
            break;
        }
        else if(c == '\n')
        {
            return std::unexpected<LexerError>{"Found newline, expecting closing quote to string"};
        }

        lexene += c;
        Consume();
    }

    if(IsAtEnd() || (!IsAtEnd() && Peek() != '"'))
    {
        return std::unexpected<LexerError>("Missing closing quote to string");
    }
    Consume(); // consume the extra quotation mark
    return Token{TokenType::String, lexene};
}

std::expected<Token, LexerError>  Lexer::LexSymbol()
{
    Consume();
    return {};
}

char Lexer::Peek() const
{
    return source.at(char_index);
}

char Lexer::PeekNext() const
{
    return source.at(char_index + 1);
}

int Lexer::Consume()
{
    return ++char_index;
}

std::expected<std::vector<Token>, LexerError> Lexer::Lex(const std::string_view source)
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
        if(std::isalpha(cur) || cur == '_')
        {
            auto result = LexIdentifier();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            tokens.push_back(result.value());
        }
        else if(std::isdigit(cur))
        {
            auto result = LexNumber();
            if (!result)
            {
                return std::unexpected(result.error());
            }

            std::println("Got number {}", result->lexeme);
            tokens.push_back(result.value());
        }
        else if (cur == '"')
        {
            auto result = LexString();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            std::println("Got string '{}'", result->lexeme);
            tokens.push_back(result.value());
        }
        else
        {
            auto result = LexSymbol();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            tokens.push_back(result.value());
        }
    }

    tokens.push_back(Token(TokenType::EndOfFile));

    return tokens;
}
