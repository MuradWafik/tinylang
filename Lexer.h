#pragma once

#include <expected>
#include <filesystem>
#include <vector>
#include <string>

#include "Token.h"

// Converts text to tokens
class Lexer
{
public:
    std::expected<std::vector<Token>, std::string> Lex(std::string_view source);
private:
    [[nodiscard]] bool IsAtEnd() const;
    [[nodiscard]] char Peek() const;

    void SkipWhitespace();

    Token LexIdentifier();
    Token LexNumber();
    Token LexString();
    Token LexSymbol();

private:
    int char_index{0};
    std::string_view source;
};
