#pragma once
#include <string>
#include <cstdint>
#include <format>
#include <unordered_map>


enum class TokenType
{
    // Literals
    Identifier,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,

    // Keywords
    Fn,
    Var,
    If,
    Else,
    While,
    Return,
    True,
    False,
    Break,
    Continue,
    For,
    In,
    Range,
    Enum,
    Native,
    Module,
    Struct,
    Self,
    Interface,
    Extend,
    Switch,

    // Types
    IntType,
    FloatType,
    BoolType,
    StringType,
    CharType,
    Void,

    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Modulo,
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
    LeftCurlyBrace,
    RightCurlyBrace,
    LeftSquareBracket,
    RightSquareBracket,
    Colon,
    Semicolon,
    Comma,
    Arrow,
    Dot,

    Underscore, // discard/default, not sure which it falls into

    EndOfFile
};



struct SourceLocation
{
    uint32_t line_number;
    uint32_t column;
};

template <>
struct std::formatter<SourceLocation> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const SourceLocation& sl, FormatContext& ctx) const {
        // Construct the string representation
        std::string s = std::format("Line: {} Column {}", sl.line_number, sl.column);

        // Delegate parsing and padding logic to the base string formatter
        return std::formatter<std::string>::format(s, ctx);
    }
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
        {"break", TokenType::Break},
        {"continue", TokenType::Continue},
        {"int", TokenType::IntType},
        {"float", TokenType::FloatType},
        {"bool", TokenType::BoolType},
        {"String", TokenType::StringType},
        {"char", TokenType::CharType},
        {"void", TokenType::Void},
        {"native", TokenType::Native},
        {"module", TokenType::Module},
        {"struct", TokenType::Struct},
        {"self", TokenType::Self},
        {"for", TokenType::For},
        {"in", TokenType::In},
        {"range", TokenType::Range},
        {"enum", TokenType::Enum},
        {"interface", TokenType::Interface},
        {"extend", TokenType::Extend},
        {"switch", TokenType::Switch},
    };
    static std::string TypeToString(TokenType token_type);

    [[nodiscard]] bool IsPrimitiveTypeName() const;
    [[nodiscard]] bool IsArithmeticOperator() const;
    [[nodiscard]] bool IsComparisonOperator() const;
    [[nodiscard]] bool IsLogicalOperator() const;
    [[nodiscard]] bool IsEqualityOperator() const;

    [[nodiscard]] bool IsNonVoidType() const
    {
        return type != TokenType::Void && (IsPrimitiveTypeName() || type == TokenType::Identifier);
    }

    [[nodiscard]] bool IsVoidableType() const
    {
        return IsPrimitiveTypeName() || type == TokenType::Identifier;
    }
};

template <>
struct std::formatter<TokenType> : std::formatter<std::string>
{
    template <typename FormatContext>
    auto format(const TokenType& t, FormatContext& ctx) const {
        return std::formatter<std::string>::format(Token::TypeToString(t), ctx);
    }
};