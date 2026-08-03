#include "frontend/Parser.h"

#include <cassert>
#include <format>

#include "frontend/Expression.h"

const Token& Parser::Consume()
{
    assert(!IsAtEnd());
    const auto& cur = Peek();
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
    return index >= tokens.size() || Peek().type == TokenType::EndOfFile;
}

// Consumes and returns true if the target is found, otherwise just returns false
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
    if(IsAtEnd() || Peek().type != expected)
    {
        SourceLocation loc = IsAtEnd() ? SourceLocation{"", 0, 0} : Peek().source_location;
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
        case TokenType::Return: return ParseReturnStatement();
        case TokenType::While: return ParseWhileStatement();
        case TokenType::If: return ParseIfStatement();
        case TokenType::LeftCurlyBrace: return ParseBodyStatement();
        case TokenType::Break: return ParseBreakStatement();
        case TokenType::Continue: return ParseContinueStatement();
        case TokenType::Native: return ParseNativeStatement();
        case TokenType::Struct: return ParseStructDeclaration();
        case TokenType::Enum: return ParseEnumDeclaration();
        case TokenType::Interface: return ParseInterfaceDeclaration();
        case TokenType::For: return ParseForLoop();
        case TokenType::Extend: return ParseExtendStatement();
        case TokenType::Export: return ParseExportStatement();
        case TokenType::Import: return ParseImportStatement();
        case TokenType::Module: return ParseModuleDeclaration();
        case TokenType::EndOfFile: return std::unexpected("Unexpected end of file");
        default: return ParseExpressionStatement();
    }
}

ExpectedPtr<VariableDeclaration> Parser::ParseVariableDeclaration()
{
    auto var = Expect(TokenType::Var, "Expected variable keyword");
    if(!var)
    {
        return std::unexpected(var.error());
    }

    auto variable_name = Expect(
        TokenType::Identifier,
        "Variable name identifier after var keyword"
    );

    if(!variable_name)
    {
        return std::unexpected(variable_name.error());
    }

    std::optional<Token> type_name = std::nullopt;
    if(Match(TokenType::Colon))
    {
        auto type_name_expected = ParseTypeName();
        if(!type_name_expected)
        {
            return std::unexpected(type_name_expected.error());
        }
        type_name = type_name_expected.value();
    }

    std::unique_ptr<Expression> initializer = nullptr;

    if(Match(TokenType::Assign))
    {
        auto initializer_result = ParseExpression();
        if(!initializer_result)
        {
            return std::unexpected(initializer_result.error()); // Bubble up parsing errors
        }
        initializer = std::move(initializer_result.value());
    }

    if(
        auto semicolon = Expect(TokenType::Semicolon, "Expected semicolon to end statement");
        !semicolon
    )
    {
        return std::unexpected(semicolon.error());
    }


    return std::make_unique<VariableDeclaration>(
        *variable_name,
        type_name,
        std::move(initializer),
        var->source_location);
}


ExpectedNodePtr Parser::ParseExpressionStatement()
{
    auto e = ParseExpression();
    if(!e) return std::unexpected(e.error());

    if(auto semicolon = Expect(TokenType::Semicolon, "In expression"); !semicolon) return std::unexpected(semicolon.error());

    SourceLocation loc = e.value()->source_location;
    return std::make_unique<ExpressionStatement>(std::move(e.value()), loc);
}

ExpectedExpressionPtr Parser::ParseExpression()
{
    return ParseAssignment();
}

ExpectedExpressionPtr Parser::ParseAssignment()
{
    ExpectedExpressionPtr left = ParseLogicalOr();
    if(!left) return left;

    if(Match(TokenType::Assign))
    {
        Token op = tokens[index - 1]; // same structure for all the following functions, cache the token of it
        // The left-hand side of an assignment must be a valid identifier
        // something like 10 = 9 should not be allowed

        Expression* target_expr = left.value().get();
        // no longer just a string as assigning can be a variable, array index, or struct property
        const bool is_valid_target = dynamic_cast<IdentifierExpression*>(target_expr) != nullptr ||
                               dynamic_cast<IndexAccess*>(target_expr) != nullptr ||
                               dynamic_cast<PropertyAccess*>(target_expr) != nullptr;
        if(!is_valid_target)
        {
            return std::unexpected(std::format("Invalid assignment target at {}", op.source_location));
        }

        // it's a right associative operator, so right must be recursive, not down the chain
        auto right = ParseAssignment();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        return std::make_unique<AssignmentExpression>(std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseSwitch()
{
    auto& switch_keyword = Consume(); // switch

    auto target = ParseExpression();
    if(!target) return target;

    constexpr auto error_msg = "Parsing switch expression";
    if(auto open = Expect(TokenType::LeftCurlyBrace, error_msg);
        !open)
    {
        return std::unexpected(open.error());
    }

    std::vector<SwitchBranch> branches{};
    while(!IsAtEnd() && Peek().type != TokenType::RightCurlyBrace)
    {
        std::unique_ptr<Expression> pattern = nullptr;

        // null type if the pattern is discard
        if(!Match(TokenType::Underscore))
        {
            auto pattern_expr = ParseExpression();
            if(!pattern_expr) return pattern_expr;

            pattern = std::move(pattern_expr.value());
        }

        if(const auto arrow = Expect(TokenType::Arrow, error_msg);
            !arrow)
        {
            return std::unexpected(arrow.error());
        }

        auto val = ParseExpression();
        if(!val) return val;

        branches.emplace_back(std::move(pattern), std::move(val.value()));

        Match(TokenType::Comma); // allow for trailing comma

    }

    if(const auto closed = Expect(TokenType::RightCurlyBrace, error_msg);
        !closed)
    {
        return std::unexpected(closed.error());
    }

    return std::make_unique<SwitchExpression>(
        std::move(target.value()),
        std::move(branches),
        switch_keyword.source_location
    );
}

ExpectedExpressionPtr Parser::ParseLogicalOr()
{
    ExpectedExpressionPtr left = ParseLogicalAnd();
    if(!left) return left;

    while(Match(TokenType::OrOr))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseLogicalAnd();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseLogicalAnd()
{
    ExpectedExpressionPtr left = ParseEquality();
    if(!left) return left;

    while(Match(TokenType::AndAnd))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseEquality();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseEquality()
{
    ExpectedExpressionPtr left = ParseComparison();
    if(!left) return left;

    while(Match(TokenType::Equal) || Match(TokenType::NotEqual))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseComparison();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseComparison()
{
    ExpectedExpressionPtr left = ParseAddition();
    if(!left) return left;

    while(Match(TokenType::LessEqual) || Match(TokenType::GreaterEqual)
        || Match(TokenType::Less) || Match(TokenType::Greater)
        || Match(TokenType::Is))
    {
        const Token& op = tokens[index - 1];
        if(op.type == TokenType::Is)
        {
            auto type_tok = Expect(TokenType::Identifier, "Expected type name after 'is'");
            if(!type_tok)
            {
                return std::unexpected(type_tok.error());
            }
            left = std::make_unique<IsExpression>(std::move(left.value()), type_tok.value(), op.source_location);
        }
        else
        {
            ExpectedExpressionPtr right = ParseAddition();
            if(!right)
            {
                return std::unexpected(right.error());
            }
            left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
        }
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseAddition()
{
    ExpectedExpressionPtr left = ParseMultiplication();
    if(!left) return left;

    while(Match(TokenType::Plus) || Match(TokenType::Minus))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseMultiplication();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseMultiplication()
{
    ExpectedExpressionPtr left = ParseCast();
    if(!left) return left;

    while(Match(TokenType::Star) || Match(TokenType::Slash) || Match(TokenType::Modulo))
    {
        const Token& op = tokens[index - 1];
        ExpectedExpressionPtr right = ParseCast();
        if(!right)
        {
            return std::unexpected(right.error());
        }
        left = std::make_unique<BinaryExpression>(op, std::move(left.value()), std::move(right.value()), op.source_location);
    }
    return left;
}

ExpectedExpressionPtr Parser::ParseCast()
{
    ExpectedExpressionPtr expr = ParseUnary();
    if(!expr) return expr;

    while(Match(TokenType::As))
    {
        const Token& op = tokens[index - 1];

        // Parse the target type on the right-hand side (e.g. 'int', 'Point', 'any')
        auto target_type_result = ParseTypeName();
        if(!target_type_result) return std::unexpected(target_type_result.error());

        expr = std::make_unique<CastExpression>(
            std::move(expr.value()),
            target_type_result.value(),
            op.source_location
        );
    }
    return expr;
}

ExpectedExpressionPtr Parser::ParseUnary()
{
    if(Peek().type == TokenType::Minus || Peek().type == TokenType::Negate)
    {
        Token op = Consume();

        // recursively call `ParseUnary` to allow for nested operators like `!!true`
        ExpectedExpressionPtr right = ParseUnary();
        if(!right)
        {
            return std::unexpected(right.error());
        }

        return std::make_unique<UnaryExpression>(op, std::move(right.value()), op.source_location);
    }
    return ParseSuffixes();
}

ExpectedExpressionPtr Parser::ParseSuffixes()
{
    ExpectedExpressionPtr expr_result = ParsePrimary();
    if(!expr_result) return expr_result;

    std::unique_ptr<Expression> expr = std::move(expr_result.value());
    // Loop continuously to catch chained suffixes like matrix[0][1]()
    while(true)
    {
        if(Match(TokenType::LeftParen)) // function call/struct init
        {
            std::vector<std::unique_ptr<Expression>> arguments;
            if(Peek().type != TokenType::RightParen)
            {
                do
                {
                    ExpectedExpressionPtr result = ParseExpression();
                    if(!result)
                    {
                        return std::unexpected(result.error());
                    }
                    arguments.push_back(std::move(result.value()));
                } while(Match(TokenType::Comma)); // more arguments
            }

            if(auto closed = Expect(TokenType::RightParen, "Expected closing parenthesis"); !closed)
            {
                return std::unexpected(closed.error());
            }

            expr = std::make_unique<CallExpression>(std::move(expr), std::move(arguments), expr->source_location);
        }
        else if(Match(TokenType::LeftSquareBracket)) // array index
        {
            auto index_expr = ParseExpression();
            if(auto closed = Expect(TokenType::RightSquareBracket, "To close off index access");
                !closed)
            {
                return std::unexpected(closed.error());
            }

            expr = std::make_unique<IndexAccess>(std::move(expr), std::move(index_expr.value()), index_expr.value()->source_location);
        }
        else if(Match(TokenType::Dot)) // property access
        {
            const auto identifier = Expect(TokenType::Identifier, "Parsing property access");
            if(!identifier)
            {
                return std::unexpected(identifier.error());
            }

            expr = std::make_unique<PropertyAccess>(std::move(expr), identifier->lexeme, identifier->source_location);
        }
        else
        {
            break;
        }
    }

    return expr;
}

ExpectedExpressionPtr Parser::ParsePrimary()
{
    switch(const auto& [type, lexeme, source_location] = Peek(); type)
    {
        case TokenType::IntLiteral:
        {
            Consume();
            return std::make_unique<IntegerLiteral>(std::stoi(lexeme), source_location);
        }
        case TokenType::FloatLiteral:
        {
            Consume();
            return std::make_unique<FloatLiteral>(std::stof(lexeme), source_location);
        }
        case TokenType::StringLiteral:
        {
            Token strToken = Consume();
            return std::make_unique<StringLiteral>(std::move(strToken.lexeme), source_location);
        }
        case TokenType::CharLiteral:
        {
            Consume();
            return std::make_unique<CharLiteral>(lexeme[0], source_location);
        }
        case TokenType::True:
        {
            Consume();
            return std::make_unique<BoolLiteral>(true, source_location);
        }
        case TokenType::False:
        {
            Consume();
            return std::make_unique<BoolLiteral>(false, source_location);
        }

        // Identifiers (Variable evaluation)
        case TokenType::Identifier:
        case TokenType::Self:
        {
            Token idToken = Consume();
            return std::make_unique<IdentifierExpression>(std::move(idToken.lexeme), source_location);
        }

        // Grouped Expressions
        case TokenType::LeftParen:
        {
            Consume(); // eat '('
            auto expr = ParseExpression();
            if(!expr) return expr;

            if(!Match(TokenType::RightParen))
            {
                return std::unexpected(std::format("Expected ')' after expression at {}", Peek().source_location));
            }
            return expr;
        }
        case TokenType::LeftSquareBracket:
        {
            Consume(); // eat '['
            std::vector<std::unique_ptr<Expression>> values;
            if(Peek().type != TokenType::RightSquareBracket)
            {
                do
                {
                    auto expr = ParseExpression();
                    if(!expr) return expr;

                    values.push_back(std::move(*expr));
                }
                while(Match(TokenType::Comma));
            }
            
            if(auto closed = Expect(TokenType::RightSquareBracket, "Expected ']' at end of array literal"); !closed)
            {
                return std::unexpected(closed.error());
            }

            return std::make_unique<ArrayLiteral>(std::move(values), source_location);
        }
        case TokenType::Switch:
        {
            return ParseSwitch();
        }

        default:
            return std::unexpected(std::format("Expected expression, found '{}' at {}", lexeme, source_location));
    }
}

ExpectedPtr<FunctionDeclaration> Parser::ParseFunctionDeclaration()
{
    // function_declaration
    // fn name(vars...? : types...) -> return type { body }
    // fn (receiver_var: receiver_type) name(vars...? : types...) -> return type { body }
    SourceLocation fn_loc = Peek().source_location;
    Consume(); // fn keyword

    std::optional<Parameter> receiver = std::nullopt;
    if(Match(TokenType::LeftParen)) // has a receiver
    {
        const auto receiver_name = Expect(TokenType::Self, "Expected receiver name");
        if(!receiver_name) return std::unexpected(receiver_name.error());

        if(const auto colon = Expect(TokenType::Colon, "Parsing receiver in function declaration");
            !colon)
        {
            return std::unexpected(colon.error());
        }

        const auto& type = Peek();
        if(!type.IsPrimitiveTypeName() && type.type != TokenType::Identifier)
        {
            return std::unexpected(std::format("Expected a valid type for receiver, got type '{}'", type.lexeme));
        }
        Consume();

        receiver = Parameter(*receiver_name, type);

        if(const auto closed = Expect(TokenType::RightParen, "Parsing receiver in function declaration");
            !closed)
        {
            return std::unexpected(closed.error());
        }
    }

    auto method_signature = ParseMethodSignature();
    if(!method_signature) return std::unexpected(method_signature.error());

    Expected<std::unique_ptr<BodyStatement>> body = ParseBodyStatement();
    if(!body)
    {
        return std::unexpected(body.error());
    }



    return std::make_unique<FunctionDeclaration>(
        std::move(method_signature.value()),
        std::move(body.value()),
        receiver,
        fn_loc
    );
}

ExpectedPtr<NativeImportStatement> Parser::ParseNativeImportDeclaration()
{
    auto name = Expect(TokenType::Identifier,"In Module name");
    if(!name)
    {
        return std::unexpected(name.error());
    }

    if(auto semicolon = Expect(TokenType::Semicolon, "Error in native module declaration");
        !semicolon)
    {
        return std::unexpected(semicolon.error());
    }
    return std::make_unique<NativeImportStatement>(name.value(), name->source_location);
}

ExpectedPtr<NativeFunctionDeclaration> Parser::ParseNativeFunctionDeclaration(const Token& native_keyword)
{
    auto method_signature = ParseMethodSignature();
    if(!method_signature) return std::unexpected(method_signature.error());
    if(auto semicolon = Expect(TokenType::Semicolon, "In native function declaration");
        !semicolon)
    {
        return std::unexpected(semicolon.error());
    }

    return std::make_unique<NativeFunctionDeclaration>(
        std::move(method_signature.value()),
        native_keyword.source_location
    );
}

ExpectedNodePtr Parser::ParseNativeStatement()
{
    auto& native_keyword = Consume();
    if(Match(TokenType::Import))
    {
        return ParseNativeImportDeclaration();
    }

    if(Match(TokenType::Fn)) // native function
    {
        return ParseNativeFunctionDeclaration(native_keyword);
    }

    return std::unexpected(std::format("Unknown token after Native keyword '{}'", Peek().type));
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
    if(Peek().type == TokenType::RightParen)
    {
        return parameters;
    }

    do
    {
        auto parameter_name = Expect(TokenType::Identifier, "Expected parameter name");
        if(!parameter_name) return std::unexpected(parameter_name.error());

        if(auto colon = Expect(TokenType::Colon, "Error after parameter name");
            !colon)
        {
            return std::unexpected(colon.error());
        }

        auto type_name_expected = ParseTypeName();
        if(!type_name_expected)
        {
            return std::unexpected(type_name_expected.error());
        }
        auto type_name = type_name_expected.value();
        
        if(!type_name.IsNonVoidType())
        {
            return std::unexpected(
                std::format("Expected typename for parameter '{}', got '{}' at {}, its type is '{}'",
                    parameter_name->lexeme,
                    type_name.lexeme,
                    type_name.source_location,
                    type_name.type)
            );
        }

        parameters.emplace_back(*parameter_name, type_name);
    } while(Match(TokenType::Comma));

    return parameters;
}

ExpectedPtr<BodyStatement> Parser::ParseBodyStatement()
{
    const Token& starter = Peek();
    if(starter.type != TokenType::LeftCurlyBrace)
    {
        return std::unexpected(
            std::format("Expected '{}' to start body statement, got '{}'. {}",
                TokenType::LeftCurlyBrace, starter.type, starter.source_location)
        );
    }
    Consume();

    SourceLocation loc = starter.source_location;
    auto body = std::make_unique<BodyStatement>(loc);
    while(!IsAtEnd() && Peek().type != TokenType::RightCurlyBrace)
    {
        ExpectedNodePtr statement = ParseStatement();
        if(!statement) return std::unexpected(statement.error());

        body->statements.push_back(std::move(statement.value()));
    }

    if(!Match(TokenType::RightCurlyBrace))
    {
        return std::unexpected(
            std::format("Unterminated block statement. Expected '{}' to match the opening brace at {}.",
                Token::TypeToString(TokenType::RightCurlyBrace),
                starter.source_location)
        );
    }

    return body;
}


ExpectedPtr<IfStatement> Parser::ParseIfStatement()
{
    auto if_keyword = Expect(TokenType::If, "If keyword expected");
    if(!if_keyword)
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

    if(Peek().type == TokenType::Else)
    {
        Consume();
        if(Peek().type == TokenType::If)
        {
            auto elif_branch = ParseIfStatement();
            if(!elif_branch)
            {
                return std::unexpected(std::format("Error parsing else if branch, {}", elif_branch.error()));
            }
            return std::make_unique<IfStatement>(
                std::move(condition.value()),
                std::move(body.value()),
                std::move(elif_branch.value()),
                if_keyword->source_location
            );
        }
        auto else_body = ParseBodyStatement();
        if(!else_body)
        {
            return std::unexpected(std::format("Error parsing else statement, {}", else_body.error()));
        }

        return std::make_unique<IfStatement>(
            std::move(condition.value()),
            std::move(body.value()),
            std::move(else_body.value()),
            if_keyword->source_location
        );
    }
    return std::make_unique<IfStatement>(std::move(condition.value()), std::move(body.value()), nullptr, if_keyword->source_location);
}

ExpectedPtr<WhileStatement> Parser::ParseWhileStatement()
{
    constexpr std::string_view error_message = "Error parsing while statement";
    auto while_keyword = Expect(TokenType::While, error_message);
    if(!while_keyword)
    {
        return std::unexpected(while_keyword.error());
    }

    if(auto left_paren = Expect(TokenType::LeftParen, error_message);
        !left_paren)
    {
        return std::unexpected(left_paren.error());
    }

    auto condition = ParseExpression();
    if(!condition)
    {
        return std::unexpected(
            std::format("Error parsing condition of while statement. {}", condition.error())
        );
    }

    if(auto right_paren = Expect(TokenType::RightParen, error_message);
        !right_paren)
    {
        return std::unexpected(right_paren.error());
    }

    auto body = ParseBodyStatement();
    if(!body)
    {
        return std::unexpected(
            std::format("Error parsing body of while statement. {}", body.error())
        );
    }

    return std::make_unique<WhileStatement>(std::move(condition.value()), std::move(body.value()), while_keyword->source_location);
}

ExpectedPtr<ReturnStatement> Parser::ParseReturnStatement()
{
    constexpr std::string_view error_msg = "Error parsing return statement";
    auto return_keyword = Expect(TokenType::Return, error_msg);
    if(!return_keyword)
    {
        return std::unexpected(return_keyword.error());
    }

    if(Match(TokenType::Semicolon))
    {
        return std::make_unique<ReturnStatement>(nullptr, return_keyword->source_location);
    }

    auto return_expression = ParseExpression();
    if(!return_expression)
    {
        return std::unexpected(std::format("{}, {}", error_msg, return_expression.error()));
    }

    if(auto semi_colon = Expect(TokenType::Semicolon, error_msg);
        !semi_colon)
    {
        return std::unexpected(semi_colon.error());
    }
    return std::make_unique<ReturnStatement>(std::move(return_expression.value()), return_keyword->source_location);
}

ExpectedPtr<BreakStatement> Parser::ParseBreakStatement()
{
    constexpr std::string_view error_msg = "Error parsing break statement";
    auto break_keyword = Expect(TokenType::Break, error_msg);
    if(!break_keyword)
    {
        return std::unexpected(break_keyword.error());
    }
    if(auto semi_colon = Expect(TokenType::Semicolon, error_msg); !semi_colon)
    {
        return std::unexpected(semi_colon.error());
    }
    return std::make_unique<BreakStatement>(break_keyword->source_location);
}

ExpectedPtr<ContinueStatement> Parser::ParseContinueStatement()
{
    constexpr std::string_view error_msg = "Error parsing continue statement";
    auto continue_keyword = Expect(TokenType::Continue, error_msg);
    if(!continue_keyword)
    {
        return std::unexpected(continue_keyword.error());
    }
    if(auto semi_colon = Expect(TokenType::Semicolon, error_msg); !semi_colon)
    {
        return std::unexpected(semi_colon.error());
    }
    return std::make_unique<ContinueStatement>(continue_keyword->source_location);
}

ExpectedPtr<StructDeclaration> Parser::ParseStructDeclaration()
{
    constexpr std::string_view error_msg = "Error parsing struct declaration";
    Consume();
    auto struct_name = Expect(TokenType::Identifier, error_msg);
    if(!struct_name)
    {
        return std::unexpected(struct_name.error());
    }

    if(const auto left_brace = Expect(TokenType::LeftCurlyBrace, error_msg); !left_brace)
    {
        return std::unexpected(left_brace.error());
    }

    std::vector<std::pair<Token, Token>> struct_members;
    
    if(Peek().type != TokenType::RightCurlyBrace)
    {
        do
        {
            if(const auto var = Expect(TokenType::Var, error_msg); !var)
            {
                return std::unexpected(var.error());
            }

            const auto var_name = Expect(TokenType::Identifier, error_msg);
            if(!var_name)
            {
                return std::unexpected(var_name.error());
            }

            if(const auto colon = Expect(TokenType::Colon, error_msg); !colon)
            {
                return std::unexpected(colon.error());
            }

            auto type_name_token = Consume();
            if(!type_name_token.IsPrimitiveTypeName() && type_name_token.type != TokenType::Identifier)
            {
                return std::unexpected(std::format(
                    "{}, expected typename got {}",
                    error_msg, Token::TypeToString(type_name_token.type))
                );
            }

            // array types
            while(Match(TokenType::LeftSquareBracket))
            {
                if(auto right_bracket = Expect(TokenType::RightSquareBracket, "Expected ']' after '[' in array type"); !right_bracket)
                {
                    return std::unexpected(right_bracket.error());
                }
                type_name_token.lexeme += "[]";
            }
            struct_members.emplace_back(*var_name, type_name_token);

            if(const auto semi_colon = Expect(TokenType::Semicolon, error_msg); !semi_colon)
            {
                return std::unexpected(semi_colon.error());
            }
        }
        while(!IsAtEnd() && Peek().type != TokenType::RightCurlyBrace);
    }

    if(const auto right_brace = Expect(TokenType::RightCurlyBrace, error_msg); !right_brace)
    {
        return std::unexpected(right_brace.error());
    }

    return std::make_unique<StructDeclaration>(*struct_name, std::move(struct_members), struct_name->source_location);
}

ExpectedPtr<EnumDeclaration> Parser::ParseEnumDeclaration()
{
    constexpr auto error = "Parsing enum declaration";
    Consume(); // enum keyword

    auto name = Expect(TokenType::Identifier, error);
    if(!name) return std::unexpected(name.error());
    if(const auto open = Expect(TokenType::LeftCurlyBrace, error); !open) return std::unexpected(open.error());

    std::vector<EnumVariant> variants{};
    if(Peek().type != TokenType::RightCurlyBrace)
    {
        do
        {
            auto name_token = Expect(TokenType::Identifier, error);
            if(!name_token) return std::unexpected(name_token.error());
            std::optional<int32_t> assignment_value = std::nullopt;
            if(Match(TokenType::Assign))
            {
                const auto val = Expect(TokenType::IntLiteral, "Parsing enum value requires a compile time int literal");
                if(!val)
                {
                    return std::unexpected(val.error());
                }

                assignment_value = std::stoi(val->lexeme);
            }

            variants.emplace_back(*name_token, assignment_value);

            // redundant but just to read
            // explicitly allow trailing comms
            if(Match(TokenType::Comma))
            {
                continue;
            }
            else
            {
                break;
            }
        }
        while(!IsAtEnd() && Peek().type != TokenType::RightCurlyBrace);
    }
    
    if(const auto right_brace = Expect(TokenType::RightCurlyBrace, error); !right_brace)
    {
        return std::unexpected(right_brace.error());
    }

    return std::make_unique<EnumDeclaration>(*name, std::move(variants), name->source_location);
}

ExpectedPtr<InterfaceDeclaration> Parser::ParseInterfaceDeclaration()
{
    auto keyword = Consume(); // interface keyword

    auto name = Expect(TokenType::Identifier, "In interface declaration");
    if(!name) return std::unexpected(name.error());

    if(auto opened = Expect(TokenType::LeftCurlyBrace, "Starting interface declaration");
        !opened)
    {
        return std::unexpected(opened.error());
    }

    std::vector<MethodSignature> methods;
    while(!IsAtEnd() && Peek().type != TokenType::RightCurlyBrace)
    {
        if(auto fn_keyword = Expect(TokenType::Fn, "For function declaration in interface");
            !fn_keyword)
        {
            return std::unexpected(fn_keyword.error());
        }

        auto signature = ParseMethodSignature();
        if(!signature) return std::unexpected(signature.error());

        if(auto semicolon = Expect(TokenType::Semicolon, "To separate interface's methods");
            !semicolon)
        {
            return std::unexpected(semicolon.error());
        }
        methods.push_back(std::move(signature.value()));
    }

    if(auto closed = Expect(TokenType::RightCurlyBrace, "Ending interface declaration");
        !closed)
    {
        return std::unexpected(closed.error());
    }
    return std::make_unique<InterfaceDeclaration>(*name, std::move(methods), keyword.source_location);
}

ExpectedPtr<ForLoop> Parser::ParseForLoop()
{
    constexpr auto error = "Parsing for loop";
    auto for_keyword = Consume(); // for keyword

    auto iterator = Expect(TokenType::Identifier, error);
    if(!iterator) return std::unexpected(iterator.error());

    if(auto in = Expect(TokenType::In, error); !in) return std::unexpected(in.error());

    auto iterable = ParseExpression();
    if(!iterable) return std::unexpected(iterable.error());

    auto body = ParseBodyStatement();
    if(!body) return std::unexpected(body.error());

    return std::make_unique<ForLoop>(
        std::move(iterator.value()), std::move(iterable.value()),
        std::move(body.value()), for_keyword.source_location);
}

ExpectedPtr<ExtendStatement> Parser::ParseExtendStatement()
{
    auto& extend = Consume(); // extend keyword

    auto target_type = Consume();
    if(!target_type.IsNonVoidType())
    {
        return std::unexpected(
            std::format("Expected type name after extend keyword, got '{}' at {}",
                target_type.lexeme,
                target_type.source_location
            )
        );
    }

    if(auto colon = Expect(TokenType::Colon, "After struct type in extend statement");
        !colon)
    {
        return std::unexpected(colon.error());
    }

    auto interface_extending = Expect(TokenType::Identifier, "Interface identifier for extend statement");

    if(!interface_extending) return std::unexpected(interface_extending.error());

    if(auto semicolon = Expect(TokenType::Semicolon, "In extend statement"); !semicolon)
    {
        return std::unexpected(semicolon.error());
    }

    return std::make_unique<ExtendStatement>(
        std::move(target_type),
        std::move(interface_extending.value()),
        extend.source_location
    );
}

ExpectedPtr<ExportableStatement> Parser::ParseExportStatement()
{
    auto export_keyword = Consume();

    std::unique_ptr<ExportableStatement> export_stmt = nullptr;
    if(Peek().type == TokenType::Fn)
    {
        auto fn_result = ParseFunctionDeclaration();
        if(!fn_result) return fn_result;

        export_stmt = std::move(fn_result.value());
    }
    else if(Peek().type == TokenType::Enum)
    {
        auto enum_result = ParseEnumDeclaration();
        if(!enum_result) return enum_result;

        export_stmt = std::move(enum_result.value());
    }
    else if(Peek().type == TokenType::Struct)
    {
        auto struct_result = ParseStructDeclaration();
        if(!struct_result) return struct_result;

        export_stmt = std::move(struct_result.value());
    }
    else if(Peek().type == TokenType::Var)
    {
        auto var_decl = ParseVariableDeclaration();
        if(!var_decl) return var_decl;

        export_stmt = std::move(var_decl.value());
    }
    else if(Peek().type == TokenType::Interface)
    {
        auto interface_decl = ParseInterfaceDeclaration();
        if(!interface_decl) return interface_decl;

        export_stmt = std::move(interface_decl.value());
    }
    else
    {
        return std::unexpected(std::format("Unknown token after export keyword '{}', at {}", Peek().lexeme, Peek().source_location));
    }

    export_stmt->is_exported = true;
    return export_stmt;
}

ExpectedPtr<ImportStatement> Parser::ParseImportStatement()
{
    auto& import_keyord = Consume(); // import

    constexpr auto error = "Parsing import statement";

    std::vector<Token> module_name; // module like std.io would be 2 identifiers (skipping the period)

    do
    {
        auto identifier = Expect(TokenType::Identifier, error);
        if(!identifier) return std::unexpected(identifier.error());
        module_name.emplace_back(std::move(identifier.value()));
    } while(!IsAtEnd() && Match(TokenType::Dot));

    if(auto closed = Expect(TokenType::Semicolon, error); !closed)
    {
        return std::unexpected(closed.error());
    }

    const auto target_name =
        module_name
        | std::views::transform([](const Token& t) { return std::string_view(t.lexeme); })
        | std::views::join_with(std::string_view{"."})
        | std::ranges::to<std::string>();

    return std::make_unique<ImportStatement>(std::move(target_name), import_keyord.source_location);
}

ExpectedPtr<ModuleDeclaration> Parser::ParseModuleDeclaration()
{
    auto& keyword = Consume();
    if(!module_name.empty())
    {
        auto next = Consume(); // to display in the error
        return std::unexpected(std::format(
            "Redefinition of module, got '{}' at {} already had '{}'",
            next.lexeme, next.source_location, module_name));
    }

    constexpr auto error = "Parsing module declaration";
    auto start = keyword.source_location;
    std::string name;
    do
    {
        auto cur = Expect(TokenType::Identifier, error);
        if(!cur)
        {
            return std::unexpected(cur.error());
        }

        name += cur->lexeme;
    } while(!IsAtEnd() && Match(TokenType::Dot));

    if(auto end = Expect(TokenType::Semicolon, error); !end) return std::unexpected(end.error());

    module_name = name;
    return std::make_unique<ModuleDeclaration>(std::move(name), start);
}

std::expected<MethodSignature, std::string> Parser::ParseMethodSignature()
{
    auto function_name = Expect(TokenType::Identifier, "Expected function name after 'fn'");
    if(!function_name) return std::unexpected(function_name.error());

    if(auto left_paren = Expect(TokenType::LeftParen, "Error after function name"); !left_paren)
    {
        return std::unexpected(left_paren.error());
    }

    auto parameters_result = ParseParameters();
    if(!parameters_result)
    {
        return std::unexpected(parameters_result.error());
    }

    if(auto right_paren = Expect(TokenType::RightParen, "Error after function parameters"); !right_paren)
    {
        return std::unexpected(right_paren.error());
    }

    if(auto arrow = Expect(TokenType::Arrow, "Error after function signature"); !arrow)
    {
        return std::unexpected(arrow.error());
    }

    auto return_type_expected = ParseTypeName();
    if(!return_type_expected)
    {
        return std::unexpected(return_type_expected.error());
    }
    auto return_type = return_type_expected.value();

    if(!return_type.IsVoidableType())
    {
        return std::unexpected(
            std::format("Expected type name after '->', got '{}' at {}",
                return_type.lexeme,
                return_type.source_location
            )
        );
    }

    return MethodSignature{
        .name = *function_name,
        .parameters = std::move(parameters_result.value()),
        .return_type = return_type
    };
}

Expected<Token> Parser::ParseTypeName()
{
    auto type_name_token = Consume();
    if(!type_name_token.IsPrimitiveTypeName() && type_name_token.type != TokenType::Identifier)
    {
        return std::unexpected(
            std::format("Expected typename, got '{}' at {}", type_name_token.type, type_name_token.source_location));
    }
    
    // Check for dot access like `geometry.Point`
    while(Match(TokenType::Dot))
    {
        auto prop = Expect(TokenType::Identifier, "Expected identifier after '.' in type name");
        if(!prop) return std::unexpected(prop.error());
        
        type_name_token.lexeme += "::" + prop->lexeme;
    }

    while(Match(TokenType::LeftSquareBracket))
    {
        if(auto right_bracket = Expect(TokenType::RightSquareBracket, "Expected ']' after '[' in array type");
            !right_bracket)
        {
            return std::unexpected(right_bracket.error());
        }
        type_name_token.lexeme += "[]";
    }

    return type_name_token;
}
