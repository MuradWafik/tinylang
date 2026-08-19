#include "frontend/Lexer.h"

#ifdef DEBUG_LEXER
#include <print>
#endif
#include <cassert>
#include <cctype>


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
    const SourceLocation startSource = {filename, line, column};
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
    TokenType token_type{TokenType::IntLiteral};
    const SourceLocation start_source = {filename, line, column};
    bool seen_dot = false;
    while (!IsAtEnd())
    {
        char c = Peek();

        if(std::isdigit(static_cast<unsigned char>(c)))
        {
            number_string += c;
            Consume();
        }
        else if(c == '.')
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

            seen_dot = true;
            token_type = TokenType::FloatLiteral;
            number_string += c;
            Consume();
        }
        else
        {
            break;
        }
    }
    return Token{token_type, number_string, start_source};
}

std::expected<Token, LexerError>  Lexer::LexString()
{
    std::string lexeme;
    const SourceLocation start_source = {filename, line, column};
    char c = Peek();

    // Consume the starting quote to not be included
    Consume();

    while(!IsAtEnd())
    {
        c = Peek();
        if(c == '"')
        {
            Consume(); // consume the extra quotation mark
            return Token{TokenType::StringLiteral, lexeme, start_source};
        }
        if(c == '\n')
        {
            return std::unexpected<LexerError>{{"Found newline, expecting closing quote to string", start_source}};
        }

        if(c == '\\')
        {
            // user wrote an escape sequence
            Consume();
            if(IsAtEnd())
            {
                return std::unexpected<LexerError>{{"File end met parsing string", start_source}};
            }
            c = Peek();

            std::optional<char> escaped = GetEscapeCharacter(c);
            if(!escaped.has_value())
            {
                return std::unexpected<LexerError>{
                    {
                        std::format("Unexpected escape character: {}", c),
                        start_source
                    }
                };
            }
            
            lexeme += escaped.value();
            Consume();
            continue;
        }

        lexeme += c;
        Consume();
    }

    return std::unexpected<LexerError>{{"Missing closing quote to string", start_source}};
}

std::expected<Token, LexerError> Lexer::LexInterpolatedString()
{
    std::string lexeme;
    const SourceLocation start_source = {filename, line, column};

    Consume();
    Consume();
    int brace_depth = 0;
    while(!IsAtEnd())
    {
        char c = Peek();
        if(c == '"' && brace_depth == 0)
        {
            Consume();
            return Token{TokenType::InterpolatedStringLiteral, lexeme, start_source};
        }

        if(c == '\n') return std::unexpected<LexerError>{{"Found newline, expecting closing quote to interpolated string", start_source}};
    
        if(c == '\\')
        {
            Consume();
            if(IsAtEnd()) return std::unexpected<LexerError>{{"File end met parsing interpolated string", start_source}};
            
            char next_char = Peek();
            std::optional<char> escaped = GetEscapeCharacter(next_char);
            if(escaped.has_value()) lexeme += escaped.value();
            else
            {
                lexeme += '\\';
                lexeme += next_char;
            }
            Consume();
            continue;
        }

        if(c == '{')
        {
            if(PeekNext() == '{' && brace_depth == 0)
            {
                lexeme += "{{";
                Consume();
                Consume();
                continue;
            }
            ++brace_depth;
        }
        else if(c == '}')
        {
            if(PeekNext() == '}' && brace_depth == 0)
            {
                lexeme += "}}";
                Consume();
                Consume();
                continue;
            }
            if(brace_depth > 0) --brace_depth;      
        }

        lexeme += c;
        Consume();
    }

    return std::unexpected<LexerError>{{"Missing closing quote to interpolated string", start_source}};
}

std::expected<Token, LexerError> Lexer::LexChar()
{
    std::string lexeme;
    const SourceLocation start_source = {filename, line, column};
    Consume(); // consume the starting quote

    if(IsAtEnd()) return std::unexpected<LexerError>{{"File ended while parsing char", start_source}};

    if(const char c = Peek(); c == '\\')
    {
        Consume();
        if(IsAtEnd()) return std::unexpected<LexerError>{{"File ended while parsing char escape", start_source}};
        
        const std::optional<char> escaped = GetEscapeCharacter(Peek());
        if(!escaped.has_value())
        {
            return std::unexpected<LexerError>{{"Invalid escape sequence in char", start_source}};
        }
        lexeme += escaped.value();
    }
    else
    {
        lexeme += c;
    }
    Consume();

    if(!Match('\''))
    {
        return std::unexpected<LexerError>{{"Missing closing quote for char", start_source}};
    }

    return Token{TokenType::CharLiteral, lexeme, start_source};
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
            return CheckSymbolForNext('&', TokenType::AndAnd, TokenType::BitAnd);
        }
        case '|':
        {
            return CheckSymbolForNext('|', TokenType::OrOr, TokenType::BitOr);
        }
        case '!':
        {
            return CheckSymbolForNext('!', '=', TokenType::NotEqual, TokenType::Negate);
        }
        case '-':
        {
            return CheckSymbolForNext('-', '>', TokenType::Arrow, TokenType::Minus);
        }
        case '%':
        {
            return ReturnSingleCharSymbol(TokenType::Modulo);
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
            return ReturnSingleCharSymbol(TokenType::LeftCurlyBrace);
        }
        case '}':
        {
            return ReturnSingleCharSymbol(TokenType::RightCurlyBrace);
        }
        case '[':
        {
            return ReturnSingleCharSymbol(TokenType::LeftSquareBracket);
        }
        case ']':
        {
            return ReturnSingleCharSymbol(TokenType::RightSquareBracket);
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
        case '.':
        {
            return ReturnSingleCharSymbol(TokenType::Dot);
        }
        default:
            const char c = Peek();
            const SourceLocation source_location{filename, line, column};
            Consume();
            return std::unexpected<LexerError>
            {
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

bool Lexer::Match(const char c)
{
    if(IsAtEnd()) return false;
    if(Peek() == c)
    {
        Consume();
        return true;
    }
    return false;
}

std::expected<std::vector<Token>, LexerError> Lexer::Lex(std::string_view source, std::string filename)
{
    this->index = 0;
    this->line = 1;
    this->column = 1;
    this->source = source;
    this->filename = filename;
    std::vector<Token> tokens;
    while (!IsAtEnd())
    {
        // ensure multiple lines of spaces/comments get skipped
        bool progress = true;
        while (progress && !IsAtEnd())
        {
            progress = false;
            size_t start_index = index;
            SkipWhitespace();
            TrySkipComments();
            if(index > start_index)
            {
                progress = true;
            }
        }
        if(IsAtEnd())
        {
            break;
        }

        const char cur = Peek();
        if(cur == '_')
        {
            if(!PeekNext() || std::isspace(PeekNext().value()) || PeekNext().value() == '{' || PeekNext().value() == '-')
            {
                SourceLocation loc{filename, line, column};
                tokens.emplace_back(TokenType::Underscore, "_", loc);

                Consume();
                continue;
            }
        }

        if(std::isalpha(static_cast<unsigned char>(cur)) || cur == '_')
        {
            auto result = LexIdentifier();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            tokens.push_back(result.value());
        }
        else if(std::isdigit(static_cast<unsigned char>(cur)))
        {
            auto result = LexNumber();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            tokens.push_back(result.value());
        }
        else if(cur == '$' && PeekNext() == '"')
        {
            auto result = LexInterpolatedString();
            if(!result) return std::unexpected(result.error());

            tokens.push_back(result.value());
        }
        else if(cur == '"')
        {
            auto result = LexString();
            if(!result)
            {
                return std::unexpected(result.error());
            }

            tokens.push_back(result.value());
        }
        else if(cur == '\'')
        {
            auto result = LexChar();
            if(!result) return std::unexpected(result.error());

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

    tokens.push_back(Token(TokenType::EndOfFile, "", {filename, line, column}));
    return tokens;
}

Token Lexer::ReturnSingleCharSymbol(const TokenType token_type)
{
    const SourceLocation startSource = {filename, line, column};
    const char c = Peek();

    const auto s = std::string(1, c);
    Consume();
    return Token{token_type, s, startSource};
}

Token Lexer::CheckSymbolForNext(
    const char before,
    const char target_next,
    const TokenType on_success,
    const TokenType on_fail)
{
    const SourceLocation startSource = {filename, line, column};
    Consume();
    if(!IsAtEnd() && Peek() == target_next)
    {
        Consume();
        // HAVE TO USE BRACES INITIALIZER FOR STRING OTHERWISE IT USES INITIALIZER LIST CONSTRUCTOR AND PLACES A 0x01 in index 0
        return {on_success, std::string(1, before) + target_next, startSource};
    }

    return {on_fail, std::string(1, before), startSource};
}

bool Lexer::TrySkipComments()
{
    if(IsAtEnd())
    {
        return false;
    }
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

std::optional<char> Lexer::GetEscapeCharacter(const char c)
{
    switch(c)
    {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '"':  return '"';
        case '\\': return '\\';
        case '0':  return '\0';
        case '\'': return '\'';
        default:   return std::nullopt;
    }
}
