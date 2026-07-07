#pragma once

#include <expected>
#include <filesystem>
#include <vector>
#include <string>

#include "Token.h"


struct LexerError
{
    std::string message;
    SourceLocation location;
};

// Converts text to tokens
class Lexer
{
public:
    std::expected<std::vector<Token>, LexerError> Lex(std::string_view source);
private:
    [[nodiscard]] bool IsAtEnd() const;
    [[nodiscard]] char Peek() const;
    [[nodiscard]] char PeekNext() const;
    int Consume();

    void SkipWhitespace();

    std::expected<Token, LexerError> LexIdentifier();
    std::expected<Token, LexerError> LexNumber();
    std::expected<Token, LexerError> LexString();
    std::expected<Token, LexerError> LexSymbol();

private:
    int char_index{0};
    std::string_view source;
};

