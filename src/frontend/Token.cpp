#include "frontend/Token.h"


std::string Token::TypeToString(const TokenType token_type)
{
    switch (token_type)
    {
        // Literals & Identifiers
        case TokenType::Identifier:      return "identifier";
        case TokenType::IntLiteral:      return "integer literal";
        case TokenType::FloatLiteral:    return "float literal";
        case TokenType::StringLiteral:   return "string literal";
        case TokenType::CharLiteral:     return "char literal";

        // Keywords
        case TokenType::Fn:              return "fn";
        case TokenType::Var:             return "var";
        case TokenType::If:              return "if";
        case TokenType::Else:            return "else";
        case TokenType::While:           return "while";
        case TokenType::Return:          return "return";
        case TokenType::True:            return "true";
        case TokenType::False:           return "false";
        case TokenType::Native:          return "native";
        case TokenType::Module:          return "module";
        case TokenType::Continue:        return "continue";
        case TokenType::Break:           return "break";
        case TokenType::Self:            return "self";
        case TokenType::Struct:          return "struct";
        case TokenType::For:             return "for";
        case TokenType::In:              return "in";
        case TokenType::Range:           return "range";
        case TokenType::Enum:            return "enum";
        case TokenType::Extend:          return "extend";
        case TokenType::Interface:       return "interface";
        case TokenType::Switch:          return "switch";

        // Built-in Types
        case TokenType::IntType:         return "int";
        case TokenType::FloatType:       return "float";
        case TokenType::BoolType:        return "bool";
        case TokenType::StringType:      return "String";
        case TokenType::CharType:        return "char";
        case TokenType::Void:            return "void";

        // Operators
        case TokenType::Plus:            return "+";
        case TokenType::Minus:           return "-";
        case TokenType::Star:            return "*";
        case TokenType::Slash:           return "/";
        case TokenType::Assign:          return "=";
        case TokenType::Modulo:          return "%";
        case TokenType::Equal:           return "==";
        case TokenType::Negate:          return "!";
        case TokenType::NotEqual:        return "!=";
        case TokenType::Less:            return "<";
        case TokenType::LessEqual:       return "<=";
        case TokenType::Greater:         return ">";
        case TokenType::GreaterEqual:    return ">=";
        case TokenType::BitAnd:          return "&";
        case TokenType::AndAnd:          return "&&";
        case TokenType::BitOr:           return "|";
        case TokenType::OrOr:            return "||";

        // Delimiters & Punctuation
        case TokenType::LeftParen:          return "(";
        case TokenType::RightParen:         return ")";
        case TokenType::LeftCurlyBrace:     return "{";
        case TokenType::RightCurlyBrace:    return "}";
        case TokenType::LeftSquareBracket:  return "[";
        case TokenType::RightSquareBracket: return "]";
        case TokenType::Colon:              return ":";
        case TokenType::Semicolon:          return ";";
        case TokenType::Comma:              return ",";
        case TokenType::Arrow:              return "->";
        case TokenType::Dot:                return ".";

        case TokenType::Underscore:         return "_";

        // Special System Tokens
        case TokenType::EndOfFile:         return "end of file";
    }
    return "unknown token";
}

bool Token::IsPrimitiveTypeName() const
{
    switch (type)
    {
        case TokenType::BoolType:
        case TokenType::IntType:
        case TokenType::FloatType:
        case TokenType::StringType:
        case TokenType::CharType:
        case TokenType::Void:
            return true;
        default: return false;
    }
}

bool Token::IsArithmeticOperator() const
{
    switch (type)
    {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
            return true;
        default: return false;
    }
}

bool Token::IsComparisonOperator() const
{
    switch(type)
    {
        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
        case TokenType::LessEqual:
            return true;
        default: return false;
    }
}

bool Token::IsLogicalOperator() const
{
    switch(type)
    {
        case TokenType::AndAnd:
        case TokenType::OrOr:
            return true;
        default: return false;
    }
}

bool Token::IsEqualityOperator() const
{
    switch(type)
    {
        case TokenType::Equal:
        case TokenType::NotEqual:
            return true;
        default: return false;
    }
}
