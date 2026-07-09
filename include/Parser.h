#pragma once
#include <expected>
#include <memory>
#include <vector>

#include "ASTNode.h"
#include "Expression.h"
#include "Token.h"

using ExpectedNode = std::expected<std::unique_ptr<ASTNode>, std::string>;
using ExpectedExpression = std::expected<std::unique_ptr<Expression>, std::string>;

// Converts the stream of tokens to an AST
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : index{0}, tokens{tokens} {};
    std::expected<std::vector<std::unique_ptr<ASTNode>>, std::string> ParseProgram();
private:
    size_t index;
    const std::vector<Token>& tokens;
    [[nodiscard]] const Token& Peek() const;
    [[nodiscard]] std::optional<const Token&> TryPeekNext() const;
    const Token& Consume();
    [[nodiscard]] bool IsAtEnd() const;
    [[nodiscard]] bool Match(TokenType target);


    // std::unique_ptr<ASTNode> ParseIntegerLiteral(const Token& token);
    ExpectedNode ParseStatement();
    ExpectedNode ParseVariableDeclaration();
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


};
