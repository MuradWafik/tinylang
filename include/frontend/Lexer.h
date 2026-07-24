#pragma once

#include <expected>
#include <string>
#include <vector>

#include "frontend/Token.h"


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
    [[nodiscard]] std::optional<char> PeekNext() const;
    char Consume();
    void SkipWhitespace();
    std::expected<Token, LexerError> LexIdentifier();
    std::expected<Token, LexerError> LexNumber();
    std::expected<Token, LexerError> LexString();
    std::expected<Token, LexerError> LexSymbol();

    [[nodiscard]] Token CheckSymbolForNext(
        char before, char target_next,
        TokenType on_success, TokenType on_fail);

    [[nodiscard]] Token CheckSymbolForNext(
        const char before_and_after,
        const TokenType on_success, const TokenType on_fail)
    {
        return CheckSymbolForNext(before_and_after, before_and_after, on_success, on_fail);
    }

    [[nodiscard]] Token ReturnSingleCharSymbol(TokenType token_type);
    bool TrySkipComments();

    std::size_t index = 0;
    std::uint32_t line = 1;
    std::uint32_t column = 1;
    std::string_view source;
};

