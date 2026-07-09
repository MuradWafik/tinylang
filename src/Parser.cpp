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

const Token* Parser::TryPeekNext() const
{
    if(index>= tokens.size() -1 )
    {
        return nullptr;
    }
    return &tokens[index+1];
}


bool Parser::IsAtEnd() const
{
    return index >= tokens.size();
}

bool Parser::Match(const TokenType target)
{
    if(IsAtEnd())
    {
        return false;
    }

    if(Peek().type == target)
    {
        Consume();
        return true;
    }
    return false;
}

Expected<Token> Parser::Expect(const TokenType expected, std::string_view context_message)
{
    if (IsAtEnd() || Peek().type != expected)
    {
        SourceLocation loc = IsAtEnd() ? SourceLocation{0, 0} : Peek().source_location;
        std::string got = IsAtEnd() ? "EOF" : Peek().lexeme;
        return std::unexpected(
            std::format("{} (Expected '{}', got '{}' at {})",
                context_message,
                Token::TypeToString(expected),
                got,
                loc
            )
        );
    }
    return Consume();
}

Expected<std::vector<std::unique_ptr<ASTNode>>> Parser::ParseProgram()
{
    std::vector<std::unique_ptr<ASTNode>> statements{};
    while(!IsAtEnd())
    {
        ExpectedNodePtr en = ParseStatement();
        if(!en)
        {
            return std::unexpected{en.error()};
        }
        statements.push_back(std::move(en.value()));
    }

    return statements;
}

ExpectedNodePtr Parser::ParseStatement()
{
    switch(Peek().type)
    {
        case TokenType::Var: return ParseVariableDeclaration();
        case TokenType::Fn: return ParseFunctionDeclaration();
            // case TokenType::Return: return ParseReturnStatement();

        case TokenType::If: return ParseIfStatement();
        case TokenType::LeftBrace: return ParseBodyStatement();
        default: return ParseExpressionStatement();
    }
}

ExpectedPtr<VariableDeclaration> Parser::ParseVariableDeclaration()
{
    if(auto var = Expect(TokenType::Var, "Expected variable keyword"); !var)
    {
        return std::unexpected(var.error());
    }

    auto variable_name = Expect(TokenType::Identifier,
        "Variable name identifier after var keyword");

    if(!variable_name)
    {
        return std::unexpected(variable_name.error());
    }

    if(
        auto colon = Expect(TokenType::Colon, "Expected colon after variable identifier");
        !colon
    )
    {
        return std::unexpected(colon.error());
    }

    const auto& type_name_token = Peek();
    if(!type_name_token.IsTypeName())
    {
        return std::unexpected(
            std::format("Expected typename for variable declaration got '{}'. {}'", type_name_token.type, type_name_token.source_location));
    }

    Consume();

    if(auto assign = Expect(TokenType::Assign, "Expected assignment for variable");
        !assign)
    {
        return std::unexpected(assign.error());
    }

    auto initializer_result = ParseExpression();
    if (!initializer_result)
    {
        return std::unexpected(initializer_result.error()); // Bubble up parsing errors
    }

    if(
        auto semicolon = Expect(TokenType::Semicolon, "Expected semicolon to end statement");
        !semicolon
    )
    {
        return std::unexpected(semicolon.error());
    }


    return std::make_unique<VariableDeclaration>(
        variable_name->lexeme,
        type_name_token.lexeme,
        std::move(initializer_result.value()));
}


ExpectedNodePtr Parser::ParseExpressionStatement()
{
    auto e = ParseExpression();
    if(!e) return std::unexpected(e.error());

    auto semicolon = Expect(TokenType::Semicolon, "Expected ';' after expression");
    if(!semicolon) return std::unexpected(semicolon.error());

    // Wrap the expression cleanly into a statement node
    return std::make_unique<ExpressionStatement>(std::move(e.value()));
}

ExpectedExpressionPtr Parser::ParseExpression()
{
    return ParseLogicalOr();
}


ExpectedExpressionPtr Parser::ParseLogicalOr()
{
    ExpectedExpressionPtr left = ParseLogicalAnd();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::OrOr))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseLogicalAnd();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseLogicalAnd()
{
    ExpectedExpressionPtr left = ParseEquality();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::AndAnd))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseEquality();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseEquality()
{
    ExpectedExpressionPtr left = ParseComparison();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Equal) || Match(TokenType::NotEqual))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseComparison();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseComparison()
{
    ExpectedExpressionPtr left = ParseAddition();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::LessEqual) || Match(TokenType::GreaterEqual)
        || Match(TokenType::Less) || Match(TokenType::Greater))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseAddition();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseAddition()
{
    ExpectedExpressionPtr left = ParseMultiplication();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Plus) || Match(TokenType::Minus))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseMultiplication();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseMultiplication()
{
    ExpectedExpressionPtr left = ParseUnary();
    if(!left)
    {
        return std::unexpected(left.error());
    }

    while(Match(TokenType::Star) || Match(TokenType::Slash))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseUnary();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()));
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseUnary()
{
    if (Peek().type == TokenType::Minus || Peek().type == TokenType::Negate)
    {
        Token op = Consume();

        // recersively call `ParseUnary` to allow for nested operators like `!!true`
        ExpectedExpressionPtr right = ParseUnary();
        if (!right)
        {
            return std::unexpected(right.error());
        }

        return std::make_unique<UnaryExpression>(op, std::move(right.value()));
    }
    return ParseFunctionCall();
}

ExpectedExpressionPtr Parser::ParseFunctionCall()
{
    const Token& cur = Peek();
    if(cur.type == TokenType::Identifier && TryPeekNext() != nullptr && TryPeekNext()->type == TokenType::LeftParen)
    {
        Token function_name = Consume();
        Consume(); // left parenthases

        std::vector<std::unique_ptr<Expression>> arguments;
        if (Peek().type != TokenType::RightParen)
        {
            do {
                ExpectedExpressionPtr result = ParseExpression();
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

ExpectedExpressionPtr Parser::ParsePrimary()
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

ExpectedStatementPtr Parser::ParseFunctionDeclaration()
{
    // function_declaration
    //    ::= "fn" IDENTIFIER "(" parameters? ")" ":" type block
    Consume(); // fn keyword

    auto function_name = Expect(TokenType::Identifier, "Expected function name after 'fn'");
    if(!function_name) return std::unexpected(function_name.error());

    auto left_paren = Expect(TokenType::LeftParen, "Error after function name");
    if(!left_paren) return std::unexpected(left_paren.error());

    auto parameters_result = ParseParameters();
    if(!parameters_result)
    {
        return std::unexpected(parameters_result.error());
    }

    auto right_paren = Expect(TokenType::RightParen, "Error after function parameters");
    if(!right_paren) return std::unexpected(right_paren.error());

    auto arrow = Expect(TokenType::Arrow, "Error after function signature");
    if(!arrow) return std::unexpected(arrow.error());

    const Token& return_type = Peek();
    if(!return_type.IsTypeName())
    {
        return std::unexpected(
            std::format("Expected type name after '->', got '{}' at {}",
                return_type.lexeme,
                return_type.source_location
            )
        );
    }
    Consume();

    Expected<std::unique_ptr<BodyStatement>> body = ParseBodyStatement();
    if(!body)
    {
        return std::unexpected(body.error());
    }

    return std::make_unique<FunctionDeclaration>(
        function_name->lexeme,
        std::move(parameters_result.value()),
        return_type.lexeme,
        std::move(body.value())
    );
}

Expected<std::vector<Parameter>> Parser::ParseParameters()
{
    /*
    parameters
        ::= parameter ( "," parameter )*
    parameter
        ::= IDENTIFIER ":" type
     */

    std::vector<Parameter> parameters{};

    // Since parameters are optional, check if there is no parameter list
    if (Peek().type == TokenType::RightParen)
    {
        return parameters;
    }

    do
    {
        auto parameter_name = Expect(TokenType::Identifier, "Expected parameter name");
        if(!parameter_name) return std::unexpected(parameter_name.error());

        auto colon = Expect(TokenType::Colon, "Error after parameter name");
        if(!colon) return std::unexpected(colon.error());

        const Token& type_name = Peek();
        if(!type_name.IsTypeName())
        {
            return std::unexpected(
                std::format("Expected type name for parameter '{}', got '{}' at {}",
                    parameter_name->lexeme,
                    type_name.lexeme,
                    type_name.source_location)
            );
        }
        Consume();

        parameters.emplace_back(parameter_name->lexeme, type_name.lexeme);
    } while(Match(TokenType::Comma));

    return parameters;
}

ExpectedPtr<BodyStatement> Parser::ParseBodyStatement()
{
    const Token& starter = Peek();
    if(starter.type != TokenType::LeftBrace)
    {
        return std::unexpected(
            std::format("Expected '{}' to start body statement, got '{}'. {}",
                TokenType::LeftBrace, starter.type, starter.source_location)
        );
    }
    Consume();

    auto body = std::make_unique<BodyStatement>();

    while(!IsAtEnd() && Peek().type != TokenType::RightBrace)
    {
        ExpectedNodePtr statement = ParseStatement();
        if(!statement) return std::unexpected(statement.error());

        body->statements.push_back(std::move(statement.value()));
    }

    if (!Match(TokenType::RightBrace))
    {
        return std::unexpected(
            std::format("Unterminated block statement. Expected '{}' to match the opening brace at {}.",
                Token::TypeToString(TokenType::RightBrace),
                starter.source_location)
        );
    }

    return body;
}


ExpectedPtr<IfStatement> Parser::ParseIfStatement()
{
    if(const auto if_keyword = Expect(TokenType::If, "If keyword expected");
        !if_keyword)
    {
        return std::unexpected(if_keyword.error());
    }

    if(const auto left_paren = Expect(TokenType::LeftParen, "Expected '('");
        !left_paren)
    {
        return std::unexpected(left_paren.error());
    }

    auto condition = ParseExpression();
    if(!condition)
    {
        return std::unexpected(condition.error());
    }

    if(const auto right_paren = Expect(TokenType::RightParen, "Missing ending condition");
    !right_paren)
    {
        return std::unexpected(right_paren.error());
    }

    auto body = ParseBodyStatement();
    if(!body)
    {
        return std::unexpected(body.error());
    }

    return std::make_unique<IfStatement>(std::move(condition.value()), std::move(body.value()));
}
