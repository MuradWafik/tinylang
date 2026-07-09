#include "Parser.h"

#include <cassert>
#include <format>

#include "Expression.h"


const Token& Parser::Consume()
{
    assert(!IsAtEnd());
    auto& cur = Peek();
    ++index;
    return cur;
}

const Token& Parser::Peek() const
{
    assert(index < tokens.size());
    return tokens[index];
}

std::optional<const Token&> Parser::TryPeekNext() const
{
    if(index>= tokens.size() -1 )
    {
        return std::nullopt;
    }
    return std::optional<const Token&>{tokens[index+1]};
}


bool Parser::IsAtEnd() const
{
    return index >= tokens.size();
}

bool Parser::Match(const TokenType target)
{
    if(Peek().type == target)
    {
        Consume();
        return true;
    }
    return false;
}


std::expected<std::vector<std::unique_ptr<ASTNode>>, std::string> Parser::ParseProgram()
{
    std::vector<std::unique_ptr<ASTNode>> statements{};
    while(!IsAtEnd())
    {
        ExpectedNode en = ParseStatement();
        if(!en)
        {
            return std::unexpected{en.error()};
        }
        statements.push_back(std::move(en.value()));
    }

    return statements;
}

ExpectedNode Parser::ParseStatement()
{
    switch(Peek().type)
    {
        case TokenType::Var: return ParseVariableDeclaration();
        default: return ParseExpressionStatement();
    }
}

ExpectedNode Parser::ParseVariableDeclaration()
{
    return std::unexpected("ParseVariableDeclaration not implemented");
}


ExpectedNode Parser::ParseExpressionStatement()
{
    auto token = Peek();
    auto e = ParseExpression();
    if(!e)
    {
        return std::unexpected(e.error());
    }

    if(!Match(TokenType::Semicolon))
    {
        return std::unexpected(std::format("No semicolon found following expression, {}", token.source_location));
    }
    return std::move(e.value());
}

ExpectedExpression Parser::ParseExpression()
{
    return ParseLogicalOr();
}


ExpectedExpression Parser::ParseLogicalOr()
{
    ExpectedExpression left = ParseLogicalAnd();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::OrOr))
    {
        ExpectedExpression right = ParseLogicalAnd();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseLogicalAnd()
{
    ExpectedExpression left = ParseEquality();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::AndAnd))
    {
        ExpectedExpression right = ParseEquality();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseEquality()
{
    ExpectedExpression left = ParseComparison();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Equal) || Match(TokenType::NotEqual))
    {
        ExpectedExpression right = ParseComparison();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseComparison()
{
    ExpectedExpression left = ParseAddition();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::LessEqual) || Match(TokenType::GreaterEqual))
    {
        ExpectedExpression right = ParseAddition();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseAddition()
{
    ExpectedExpression left = ParseMultiplication();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Plus) || Match(TokenType::Minus))
    {
        ExpectedExpression right = ParseMultiplication();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseMultiplication()
{
    ExpectedExpression left = ParseUnary();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Star) || Match(TokenType::Slash))
    {
        ExpectedExpression right = ParseUnary();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(Consume(), std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpression Parser::ParseUnary()
{
    if (Peek().type == TokenType::Minus || Peek().type == TokenType::Negate)
    {
        Token op = Consume();

        // recersively call `ParseUnary` to allow for nested operators like `!!true`
        ExpectedExpression right = ParseUnary();
        if (!right)
        {
            return std::unexpected(right.error());
        }

        return std::make_unique<UnaryExpression>(op, std::move(right.value()));
    }
    return ParseFunctionCall();
}

ExpectedExpression Parser::ParseFunctionCall()
{
    const Token& cur = Peek();
    if(cur.type == TokenType::Identifier && TryPeekNext().has_value() && TryPeekNext()->type == TokenType::LeftParen)
    {
        Token function_name = Consume();
        Consume(); // left parenthases

        std::vector<std::unique_ptr<Expression>> arguments;
        if (Peek().type != TokenType::RightParen)
        {
            do {
                ExpectedExpression result = ParseExpression();
                if(!result)
                {
                    return std::unexpected(result.error());
                }
                arguments.push_back(std::move(result.value()));
            } while (Match(TokenType::Comma)); // more arguments
        }

        if (!Match(TokenType::RightParen))
        {
            return std::unexpected(
                std::format(
                    "No closing parenthesis for invocation of function {}, at {}",
                    function_name.lexeme, function_name.source_location
                )
            );
        }

        return std::make_unique<CallExpression>(std::move(function_name.lexeme), std::move(arguments));
    }

    return ParsePrimary();
}

ExpectedExpression Parser::ParsePrimary()
{
    const auto& [type, lexeme, source_location] = Peek();

    switch (type)
    {
        // 1. Literals
        case TokenType::IntLiteral:
        {
            Consume();
            return std::make_unique<IntegerLiteral>(std::stoi(lexeme));
        }
        case TokenType::FloatLiteral:
        {
            Consume();
            return std::make_unique<FloatLiteral>(std::stof(lexeme));
        }
        case TokenType::StringLiteral:
        {
            Token strToken = Consume();
            return std::make_unique<StringLiteral>(std::move(strToken.lexeme));
        }
        case TokenType::True:
        {
            Consume();
            return std::make_unique<BoolLiteral>(true);
        }
        case TokenType::False:
        {
            Consume();
            return std::make_unique<BoolLiteral>(false);
        }

        // 2. Identifiers (Variable evaluation)
        case TokenType::Identifier:
        {
            Token idToken = Consume();
            return std::make_unique<IdentifierExpression>(std::move(idToken.lexeme));
        }

        // 3. Grouped Expressions
        case TokenType::LeftParen:
        {
            Consume(); // eat '('
            auto expr = ParseExpression();
            if (!expr) return expr;

            if (!Match(TokenType::RightParen))
            {
                return std::unexpected(std::format("Expected ')' after expression at {}", Peek().source_location));
            }
            return expr;
        }

        default:
            return std::unexpected(std::format("Expected expression, found '{}' at {}", lexeme, source_location));
    }
}