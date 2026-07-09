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

template<typename T>
using ExpectedPtr = Expected<std::unique_ptr<T>>;

using ExpectedNodePtr = ExpectedPtr<ASTNode>;
using ExpectedExpressionPtr = ExpectedPtr<Expression>;
using ExpectedStatementPtr = ExpectedPtr<Statement>;

// Converts the stream of tokens to an AST
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : index{0}, tokens{tokens} {};
    Expected<std::vector<std::unique_ptr<ASTNode>>> ParseProgram();
private:
    size_t index;
    const std::vector<Token>& tokens;
    [[nodiscard]] const Token& Peek() const;
    [[nodiscard]] const Token* TryPeekNext() const;
    const Token& Consume();
    [[nodiscard]] bool IsAtEnd() const;
    [[nodiscard]] bool Match(TokenType target);
    Expected<Token> Expect(TokenType expected, std::string_view context_message);


    ExpectedNodePtr ParseStatement();
    ExpectedNodePtr ParseExpressionStatement();
    ExpectedExpressionPtr ParseExpression();


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
    ExpectedExpressionPtr ParseLogicalOr();
    ExpectedExpressionPtr ParseLogicalAnd();
    ExpectedExpressionPtr ParseEquality();
    ExpectedExpressionPtr ParseComparison();
    ExpectedExpressionPtr ParseAddition();
    ExpectedExpressionPtr ParseMultiplication();
    ExpectedExpressionPtr ParseUnary();
    ExpectedExpressionPtr ParseFunctionCall();
    ExpectedExpressionPtr ParsePrimary(); // literals/identifiers
    //TODO: Most have the same logic refactor, (potentially a dictionary with the tokens to match and next level)


    ExpectedStatementPtr ParseFunctionDeclaration();
    Expected<std::vector<Parameter>> ParseParameters();
    ExpectedPtr<VariableDeclaration> ParseVariableDeclaration();
    ExpectedPtr<BodyStatement> ParseBodyStatement();
    ExpectedPtr<IfStatement> ParseIfStatement();

};
