#include "Token.h"


std::string Token::TypeToString(const TokenType token_type)
{
    switch (token_type)
    {
        // Literals & Identifiers
        case TokenType::Identifier:      return "identifier";
        case TokenType::IntLiteral:      return "integer literal";
        case TokenType::FloatLiteral:    return "float literal";
        case TokenType::StringLiteral:   return "string literal";

        // Keywords
        case TokenType::Fn:              return "'fn'";
        case TokenType::Var:             return "'var'";
        case TokenType::If:              return "'if'";
        case TokenType::Else:            return "'else'";
        case TokenType::While:           return "'while'";
        case TokenType::Return:          return "'return'";
        case TokenType::True:            return "'true'";
        case TokenType::False:           return "'false'";

        // Built-in Types
        case TokenType::IntType:         return "'int'";
        case TokenType::FloatType:       return "'float'";
        case TokenType::BoolType:        return "'bool'";
        case TokenType::StringType:      return "'String'";
        case TokenType::Void:            return "'void'";

        // Operators
        case TokenType::Plus:            return "'+'";
        case TokenType::Minus:           return "'-'";
        case TokenType::Star:            return "'*'";
        case TokenType::Slash:           return "'/'";
        case TokenType::Assign:          return "'='";
        case TokenType::Equal:           return "'=='";
        case TokenType::Negate:          return "'!'";
        case TokenType::NotEqual:        return "'!='";
        case TokenType::Less:            return "'<'";
        case TokenType::LessEqual:       return "'<='";
        case TokenType::Greater:         return "'>'";
        case TokenType::GreaterEqual:    return "'>='";
        case TokenType::BitAnd:          return "'&'";
        case TokenType::AndAnd:          return "'&&'";
        case TokenType::BitOr:           return "'|'";
        case TokenType::OrOr:            return "'||'";

        // Delimiters & Punctuation
        case TokenType::LeftParen:       return "'('";
        case TokenType::RightParen:      return "')'";
        case TokenType::LeftBrace:       return "'{'";
        case TokenType::RightBrace:      return "'}'";
        case TokenType::LeftBracket:     return "'['";
        case TokenType::RightBracket:    return "']'";
        case TokenType::Colon:           return "':'";
        case TokenType::Semicolon:       return "';'";
        case TokenType::Comma:           return "','";
        case TokenType::Arrow:           return "'->'";

        // Special System Tokens
        case TokenType::EndOfFile:       return "end of file";
    }
    return "unknown token";
}