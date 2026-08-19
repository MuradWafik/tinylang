#pragma once
#include <expected>
#include <memory>
#include <vector>

#include "frontend/ASTNode.h"
#include "frontend/Expression.h"
#include "frontend/Statement.h"
#include "frontend/Token.h"


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

    std::string module_name;
private:
    size_t index;
    const std::vector<Token>& tokens;
    [[nodiscard]] const Token& Peek() const;
    [[nodiscard]] const Token* TryPeekNext() const;
    const Token& Consume();
    [[nodiscard]] bool IsAtEnd() const;
    bool Match(TokenType target);
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
    ExpectedExpressionPtr ParseAssignment();
    ExpectedExpressionPtr ParseSwitch();
    ExpectedExpressionPtr ParseLogicalOr();
    ExpectedExpressionPtr ParseLogicalAnd();
    ExpectedExpressionPtr ParseEquality();
    ExpectedExpressionPtr ParseComparison();
    ExpectedExpressionPtr ParseAddition();
    ExpectedExpressionPtr ParseCast();
    ExpectedExpressionPtr ParseMultiplication();
    ExpectedExpressionPtr ParseUnary();
    ExpectedExpressionPtr ParseSuffixes();
    ExpectedExpressionPtr ParsePrimary(); // literals/identifiers
    ExpectedExpressionPtr ParseInterpolatedString(const Token& token);
    //TODO: Most have the same logic refactor, (potentially a dictionary with the tokens to match and next level)


    ExpectedPtr<FunctionDeclaration> ParseFunctionDeclaration();
    ExpectedNodePtr ParseNativeStatement(); // native module/function declaration
    Expected<std::vector<Parameter>> ParseParameters();
    ExpectedPtr<VariableDeclaration> ParseVariableDeclaration();
    ExpectedPtr<BodyStatement> ParseBodyStatement();
    ExpectedPtr<IfStatement> ParseIfStatement();
    ExpectedPtr<WhileStatement> ParseWhileStatement();
    ExpectedPtr<ReturnStatement> ParseReturnStatement();
    ExpectedPtr<BreakStatement> ParseBreakStatement();
    ExpectedPtr<ContinueStatement> ParseContinueStatement();
    ExpectedPtr<StructDeclaration> ParseStructDeclaration();
    ExpectedPtr<EnumDeclaration> ParseEnumDeclaration();
    ExpectedPtr<InterfaceDeclaration> ParseInterfaceDeclaration();
    ExpectedPtr<ForLoop> ParseForLoop();
    ExpectedPtr<ExtendStatement> ParseExtendStatement();
    ExpectedPtr<ExportableStatement> ParseExportStatement();
    ExpectedPtr<ImportStatement> ParseImportStatement();
    ExpectedPtr<ModuleDeclaration> ParseModuleDeclaration();


    ExpectedPtr<NativeImportStatement> ParseNativeImportDeclaration();
    ExpectedPtr<NativeFunctionDeclaration> ParseNativeFunctionDeclaration(const Token& native_keyword);
    std::expected<MethodSignature, std::string> ParseMethodSignature();
    Expected<Token> ParseTypeName();
};
