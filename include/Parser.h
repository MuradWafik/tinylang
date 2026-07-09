#pragma once
#include <expected>
#include <memory>
#include <vector>

#include "ASTNode.h"
#include "Expression.h"
#include "Statement.h"
#include "Token.h"


template<typename T>
using Expected = std::expected<T, std::string>;

using ExpectedNode = Expected<std::unique_ptr<ASTNode>>;
using ExpectedExpression = Expected<std::unique_ptr<Expression>>;
using ExpectedStatement = Expected<std::unique_ptr<Statement>>;

// Converts the stream of tokens to an AST
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : index{0}, tokens{tokens} {};
    std::expected<std::vector<std::unique_ptr<ASTNode>>, std::string> ParseProgram();
private:
    size_t index;
    const std::vector<Token>& tokens;
    [[nodiscard]] const Token& Peek() const;
    [[nodiscard]] const Token* TryPeekNext() const;
    const Token& Consume();
    [[nodiscard]] bool IsAtEnd() const;
    [[nodiscard]] bool Match(TokenType target);
    Expected<Token> Expect(TokenType expected, std::string_view context_message);



    // std::unique_ptr<ASTNode> ParseIntegerLiteral(const Token& token);
    ExpectedNode ParseStatement();
    ExpectedStatement ParseVariableDeclaration();
    ExpectedNode ParseExpressionStatement();
    ExpectedExpression ParseExpression();


    /* Rough overview
    *left = ParseHigherPrecedence();
    *while (next token is one of my operators)
    *   operator = consume();
    *   right = ParseHigherPrecedence();
    *   left = BinaryExpression(left, operator, right);
    *}
    *return left;
     */
    // expressions sorted from lowest to highest priority so each one calls on the one below it
    ExpectedExpression ParseLogicalOr();
    ExpectedExpression ParseLogicalAnd();
    ExpectedExpression ParseEquality();
    ExpectedExpression ParseComparison();
    ExpectedExpression ParseAddition();
    ExpectedExpression ParseMultiplication();
    ExpectedExpression ParseUnary();
    ExpectedExpression ParseFunctionCall();
    ExpectedExpression ParsePrimary(); // literals/identifiers
    //TODO: Most have the same logic refactor, (potentially a dictionary with the tokens to match and next level)


    ExpectedStatement ParseFunctionDeclaration();
    std::expected<std::vector<Parameter>, std::string> ParseParameters();


    Expected<std::unique_ptr<BodyStatement>> ParseBodyStatement();

};
