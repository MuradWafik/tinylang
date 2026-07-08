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
    while(!IsAtEnd() && std::isspace(static_cast<unsigned char>(Peek())))
    {
        Consume();
    }
}

std::expected<Token, LexerError>  Lexer::LexIdentifier()
{
    std::string lexeme = "";
    while(!IsAtEnd())
    {
        const char c = Peek();
        if(!std::isalpha(c) && c != '_')
        {
            break;
        }

        lexeme += c;
        Consume();
    }

    TokenType token_type;
    if(Token::keywords.contains(lexeme))
    {
        token_type = Token::keywords.at(lexeme);
    }
    else
    {
        token_type = TokenType::Identifier;
    }
    return Token{token_type, lexeme};
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
    /*
    * Longer symbols:
    * ==
    * <=
    * >=
    * //
    * &&
    * ||
    * !=
    * ->
    */
    switch(Peek())
    {
        case '=':
        {
            return CheckSymbolForNext('=', TokenType::Equal, TokenType::Assign);
        }
        case '<':
        {
            return CheckSymbolForNext('<', '=', TokenType::LessEqual, TokenType::Less);
        }
        case '>':
        {
            return CheckSymbolForNext('>', '=', TokenType::GreaterEqual, TokenType::Greater);
        }
        case '/':
        {
            return CheckSymbolForNext('/', TokenType::DoubleSlash, TokenType::Slash);
        }
        case '&':
        {
            return CheckSymbolForNext('&', TokenType::BitAnd, TokenType::AndAnd);
        }
        case '|':
        {
            return CheckSymbolForNext('|', TokenType::BitOr, TokenType::OrOr);
        }
        case '!':
        {
            return CheckSymbolForNext('!', '=', TokenType::Negate, TokenType::NotEqual);
        }
        case '-':
        {
            return CheckSymbolForNext('-', '>', TokenType::Minus, TokenType::Arrow);
        }
        case '+':
        {
            return ReturnSingleCharSymbol(TokenType::Plus);
        }
        case '*':
        {
            return ReturnSingleCharSymbol(TokenType::Star);
        }
        case '(':
        {
            return ReturnSingleCharSymbol(TokenType::LeftParen);
        }
        case ')':
        {
            return ReturnSingleCharSymbol(TokenType::RightParen);
        }
        case '{':
        {
            return ReturnSingleCharSymbol(TokenType::LeftBrace);
        }
        case '}':
        {
            return ReturnSingleCharSymbol(TokenType::RightBrace);
        }
        case '[':
        {
            return ReturnSingleCharSymbol(TokenType::LeftBracket);
        }
        case ']':
        {
            return ReturnSingleCharSymbol(TokenType::RightBracket);
        }
        case ':':
        {
            return ReturnSingleCharSymbol(TokenType::Colon);
        }
        case ';':
        {
            return ReturnSingleCharSymbol(TokenType::Semicolon);
        }
        case ',':
        {
            return ReturnSingleCharSymbol(TokenType::Comma);
        }
        default:
            const char c = Peek();
            Consume();
            return std::unexpected<LexerError>{
                std::format("Unexpected symbol character: {}", c)
            };
    }
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
        if(std::isalpha(cur) || cur == '_')
        {
            auto result = LexIdentifier();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            std::string fmt = Token::keywords.contains(result->lexeme) ?
                "Got keyword: {}" :
                "Got identifier: {}";
            std::println(std::runtime_format(fmt), result->lexeme);
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

            std::println("Got symbol: {}", result->lexeme);
            tokens.push_back(result.value());
        }
    }

    tokens.push_back(Token(TokenType::EndOfFile));

    return tokens;
}


Token Lexer::ReturnSingleCharSymbol(const TokenType token_type)
{
    const char c = Peek();
    Consume();
    return Token{ token_type, std::string{1, c}};
}

Token Lexer::CheckSymbolForNext(
    const char before,
    const char target_next,
    const TokenType on_success,
    const TokenType on_fail)
{
    Consume();
    if(!IsAtEnd() && Peek() == target_next)
    {
        Consume();
        return {on_success, std::string{1, before} + target_next};
    }

    return {on_fail, std::string{1, before}};

}
