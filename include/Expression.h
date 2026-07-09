#pragma once
#include <string>
#include <vector>

#include "ASTNode.h"
#include "Token.h"

// Something that has a value
// even something like x + 5 is an expression since that yields a value
// According to AI, something like foo(10) is *usually* an expression
struct Expression : ASTNode
{};


struct IntegerLiteral : Expression
{
    int value;
    explicit IntegerLiteral(const int val) : value(val) {}
};

struct FloatLiteral : Expression
{
    float value;
    explicit FloatLiteral(const float val) : value(val) {}
};

struct StringLiteral : Expression
{
    std::string value;
    explicit StringLiteral(std::string&& val) : value(std::move(val)) {}
};

struct BoolLiteral : Expression
{
    bool value;
    explicit BoolLiteral(const bool val) : value(val) {}
};

struct BinaryExpression : Expression
{
    const Token& operator_token;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpression(const Token& token, std::unique_ptr<Expression>&& left_node, std::unique_ptr<Expression>&& right_node)
        : operator_token(token),
          left(std::move(left_node)),
          right(std::move(right_node))
    {}
};

struct UnaryExpression : Expression
{
    const Token& operator_token;
    std::unique_ptr<Expression> right;

    UnaryExpression(const Token& token, std::unique_ptr<Expression>&& right_node)
        : operator_token(token),
          right(std::move(right_node))
    {}
};

struct CallExpression : Expression
{
    std::string function_name;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression(std::string&& name, std::vector<std::unique_ptr<Expression>>&& args)
        : function_name(std::move(name)),
          arguments(std::move(args)) {}
};

struct IdentifierExpression : Expression
{
    std::string name;
    explicit IdentifierExpression(std::string&& name) : name(std::move(name)) {}
};