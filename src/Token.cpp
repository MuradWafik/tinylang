#include "Token.h"


std::string Token::TypeToString(const TokenType token_type)
{
    switch (token_type)
    {
        case TokenType::Identifier:   return "Identifier";
        case TokenType::Integer:      return "Integer";
        case TokenType::Float:        return "Float";
        case TokenType::String:       return "String";

        case TokenType::Fn:           return "Fn";
        case TokenType::Var:          return "Var";
        case TokenType::If:           return "If";
        case TokenType::Else:         return "Else";
        case TokenType::While:        return "While";
        case TokenType::Return:       return "Return";
        case TokenType::True:         return "True";
        case TokenType::False:        return "False";

        case TokenType::IntType:      return "Int";
        case TokenType::FloatType:    return "FloatType";
        case TokenType::BoolType:     return "Bool";
        case TokenType::StringType:   return "StringType";
        case TokenType::Void:         return "Void";

        case TokenType::Plus:         return "Plus";
        case TokenType::Minus:        return "Minus";
        case TokenType::Star:         return "Star";
        case TokenType::Slash:        return "Slash";
        case TokenType::Assign:       return "Assign";
        case TokenType::Equal:        return "Equal";
        case TokenType::Negate:       return "Negate";
        case TokenType::NotEqual:     return "NotEqual";
        case TokenType::Less:         return "Less";
        case TokenType::LessEqual:    return "LessEqual";
        case TokenType::Greater:      return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::BitAnd:       return "BitAnd";
        case TokenType::AndAnd:       return "AndAnd";
        case TokenType::BitOr:        return "BitOr";
        case TokenType::OrOr:         return "OrOr";

        case TokenType::LeftParen:    return "LeftParen";
        case TokenType::RightParen:   return "RightParen";
        case TokenType::LeftBrace:    return "LeftBrace";
        case TokenType::RightBrace:   return "RightBrace";
        case TokenType::LeftBracket:  return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::Colon:        return "Colon";
        case TokenType::Semicolon:    return "Semicolon";
        case TokenType::Comma:        return "Comma";
        case TokenType::Arrow:        return "Arrow";

        case TokenType::EndOfFile:    return "EndOfFile";
    }
    return "<unknown TokenType>";
}
