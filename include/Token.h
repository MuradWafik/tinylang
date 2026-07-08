#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>


enum class TokenType
{
    // Literals
    Identifier,
    Integer,
    Float,
    String,

    // Keywords
    Fn,
    Var,
    If,
    Else,
    While,
    Return,
    True,
    False,

    // Types
    IntType,
    FloatType,
    BoolType,
    StringType,
    Void,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Assign,
    Equal,
    Negate,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    BitAnd,
    AndAnd,
    BitOr,
    OrOr,

    // Delimiters
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Colon,
    Semicolon,
    Comma,
    Arrow,

    EndOfFile
};

struct SourceLocation
{
    uint32_t line_number;
    uint32_t column;
};

struct Token {
    TokenType type;
    std::string lexeme;
    SourceLocation source_location;

    static inline const std::unordered_map<std::string, TokenType> keywords =
    {
        {"fn", TokenType::Fn},
        {"var", TokenType::Var},
        {"if", TokenType::If},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"return", TokenType::Return},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"int", TokenType::IntType},
        {"float", TokenType::FloatType},
        {"bool", TokenType::BoolType},
        {"String", TokenType::StringType},
        {"void", TokenType::Void},
    };
    static std::string TypeToString(TokenType token_type);
};
