#pragma once
#include <string>
#include <vector>

#include "ASTNode.h"
#include "Token.h"
#include "Type.h"

// Something that has a value
// even something like x + 5 is an expression since that yields a value
// According to AI, something like foo(10) is *usually* an expression
struct Expression : ASTNode
{
    const Type* type_info = nullptr; // reference to the one once semantic analysis occurs
};

struct IntegerLiteral final : Expression
{
    int value;
    explicit IntegerLiteral(const int val) : value(val) {}
    [[nodiscard]] std::string GetTypeString() const override { return std::format("IntegerLiteral({})", value); }
};

struct FloatLiteral final : Expression
{
    float value;
    explicit FloatLiteral(const float val) : value(val) {}
    [[nodiscard]] std::string GetTypeString() const override { return std::format("FloatLiteral({})", value); }
};

struct StringLiteral final : Expression
{
    std::string value;
    explicit StringLiteral(std::string val) : value(std::move(val)) {}
    [[nodiscard]] std::string GetTypeString() const override { return std::format("StringLiteral(\"{}\")", value); }
};

struct BoolLiteral final : Expression
{
    bool value;
    explicit BoolLiteral(const bool val) : value(val) {}
    [[nodiscard]] std::string GetTypeString() const override { return std::format("BoolLiteral({})", value); }
};

struct BinaryExpression final : Expression
{
    Token operator_token;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpression(Token token, std::unique_ptr<Expression> left_node, std::unique_ptr<Expression> right_node)
        : operator_token(std::move(token)),
          left(std::move(left_node)),
          right(std::move(right_node))
    {}
    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Binary Expression, operator({}), left: {}, right: {}", operator_token.type, left.get(), right.get());
    }
};

struct UnaryExpression final : Expression
{
    Token operator_token;
    std::unique_ptr<Expression> right;

    UnaryExpression(Token token, std::unique_ptr<Expression> right_node)
        : operator_token(std::move(token)),
          right(std::move(right_node))
    {}

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Unary Expression, operator({}), right: {}", operator_token.type, right.get());
    }
};

struct CallExpression final : Expression
{
    std::string function_name;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression(std::string name, std::vector<std::unique_ptr<Expression>> args)
        : function_name(std::move(name)),
          arguments(std::move(args)) {}

    [[nodiscard]] std::string GetTypeString() const override
    {
        std::string args_str;
        for (size_t i = 0; i < arguments.size(); ++i) {
            args_str += arguments[i] ? arguments[i]->GetTypeString() : "nullptr";
            if (i + 1 < arguments.size()) args_str += ", ";
        }
        return std::format("CallExpression(name: \"{}\", args: [{}])", function_name, args_str);
    }
};

struct IdentifierExpression final : Expression
{
    std::string name;
    explicit IdentifierExpression(std::string name) : name(std::move(name)) {}

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Identifier, name(\"{}\")", name);
    }
};

struct AssignmentExpression final : Expression
{
    std::string name;
    std::unique_ptr<Expression> value;
    AssignmentExpression(std::string name, std::unique_ptr<Expression> val)
        : name(std::move(name)), value(std::move(val)) {}
    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("AssignmentExpression(name: \"{}\", value: {})", name, value.get());
    }
};