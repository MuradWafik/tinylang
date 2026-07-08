#include "Lexer.h"

#include <cassert>
#include <cctype>
#include <print>


bool Lexer::IsAtEnd() const
{
    return index >= source.size();
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
    const SourceLocation startSource = {line, column};
    std::string lexeme = "";
    while(!IsAtEnd())
    {
        const char c = Peek();
        // identifiers have to start with letter or _, but can contain numbers afterwards
        if(!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
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
    return Token{token_type, lexeme, startSource};
}

std::expected<Token, LexerError>  Lexer::LexNumber()
{
    std::string number_string = "";
    TokenType token_type{TokenType::Int};
    const SourceLocation start_source = {line, column};
    char c = Peek();
    bool seen_dot = false;
    while(!IsAtEnd() && (std::isdigit(static_cast<unsigned char>(c)) || c == '.'))
    {
        if(c == '.')
        {
            if(seen_dot)
            {
                return std::unexpected<LexerError>
                {
                    {
                        std::format("Multiple dots found in float {}", number_string + c),
                        start_source
                    }
                };
            }
            else
            {
                token_type = TokenType::Float;
                seen_dot = true;
            }
        }

        number_string += c;
        const auto next = PeekNext();
        if(!next.has_value())
        {
            return Token{token_type, number_string, start_source};
        }
        c = next.value();
        Consume();
    }
    return Token{token_type, number_string, start_source};
}

std::expected<Token, LexerError>  Lexer::LexString()
{
    std::string lexeme = "";
    const SourceLocation start_source = {line, column};
    char c = Peek();
    assert(c == '"');

    // Consume the starting quote to not be included
    Consume();

    while(!IsAtEnd())
    {
        c = Peek();
        if(c == '"')
        {
            break;
        }
        else if(c == '\\')
        {
            // user wrote an escape sequence
            Consume();
            c = Peek();
            if(IsAtEnd())
            {
                return std::unexpected<LexerError>{{"File end met parsing string", start_source}};
            }

            switch(c)
            {
                case 'n':
                {
                    lexeme += '\n';
                    break;
                }
                case 't':
                {
                    lexeme += '\t';
                    break;
                }
                case 'r':
                {
                    lexeme += '\r';
                    break;
                }
                case '"':
                {
                    lexeme += '"';
                    break;
                }
                case '\\':
                {
                    lexeme += '\\';
                    break;
                }
                default:
                    return std::unexpected<LexerError>{
                        {
                            std::format("Unexpected escape character: {}", c),
                            start_source
                        }
                    };
            }

        }
        else if(c == '\n')
        {
            return std::unexpected<LexerError>{{"Found newline, expecting closing quote to string", start_source}};
        }

        lexeme += c;
        Consume();
    }

    if(IsAtEnd() ||  Peek() != '"')
    {
        return std::unexpected<LexerError>{{"Missing closing quote to string", start_source}};
    }

    Consume(); // consume the extra quotation mark
    return Token{TokenType::String, lexeme, start_source};
}

std::expected<Token, LexerError>  Lexer::LexSymbol()
{
    /*
    * Longer symbols:
    * ==
    * <=
    * >=
    * &&
    * ||
    * !=
    * ->
    * // ( comments are ignored, not treated as symbols
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
        case '/':
        {
            return ReturnSingleCharSymbol(TokenType::Slash);
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
            SourceLocation source_location{line, column};
            Consume();
            return std::unexpected<LexerError>{
                {
                    std::format("Unexpected symbol character: {}", c),
                    source_location
                }
            };
    }
}

char Lexer::Peek() const
{
    return source.at(index);
}

std::optional<char> Lexer::PeekNext() const
{
    if(index + 1 >= source.size())
    {
        return std::nullopt;
    }

    return source.at(index + 1);
}

char Lexer::Consume()
{
    const char cur = Peek();
    ++index;
    if(cur == '\n')
    {
        ++line;
        column = 1;
    }
    else
    {
        ++column;
    }
    return cur;
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

        if(TrySkipComments())
        {
            SkipWhitespace();
        }

        const char cur = Peek();
        if(std::isalpha(static_cast<unsigned char>(cur)) || cur == '_')
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
        else if(std::isdigit(static_cast<unsigned char>(cur)))
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
    const SourceLocation startSource = {line, column};
    const char c = Peek();
    Consume();
    return Token{token_type, std::string{1, c}, startSource};
}

Token Lexer::CheckSymbolForNext(
    const char before,
    const char target_next,
    const TokenType on_success,
    const TokenType on_fail)
{
    const SourceLocation startSource = {line, column};
    Consume();
    if(!IsAtEnd() && Peek() == target_next)
    {
        Consume();
        return {on_success, std::string{1, before} + target_next, startSource};
    }

    return {on_fail, std::string{1, before}, startSource};
}

bool Lexer::TrySkipComments()
{
    char c = Peek();
    const std::optional<char> next = PeekNext();
    if(c != '/' || !next.has_value() || next.value() != '/')
    {
        return false;
    }

    Consume();
    while(!IsAtEnd())
    {
        c = Consume();
        if(c == '\n')
        {
            break;
        }
    }

    return true;
}

