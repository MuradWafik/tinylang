#pragma once
#include <string>
#include <vector>

#include "frontend/ASTNode.h"
#include "frontend/Token.h"
#include "analysis/Type.h"

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
    explicit IntegerLiteral(const int val, const SourceLocation loc) : value(val) { this->source_location = loc; }
    [[nodiscard]] std::string GetTypeString() const override { return std::format("IntegerLiteral({})", value); }
};

struct FloatLiteral final : Expression
{
    float value;
    explicit FloatLiteral(const float val, const SourceLocation loc) : value(val) { this->source_location = loc; }
    [[nodiscard]] std::string GetTypeString() const override { return std::format("FloatLiteral({})", value); }
};

struct StringLiteral final : Expression
{
    std::string value;
    explicit StringLiteral(std::string val, const SourceLocation loc) : value(std::move(val)) { this->source_location = loc; }
    [[nodiscard]] std::string GetTypeString() const override { return std::format("StringLiteral(\"{}\")", value); }
};

struct CharLiteral final : Expression
{
    char value;
    explicit CharLiteral(const char val, const SourceLocation loc) : value(val) { this->source_location = loc; }
    [[nodiscard]] std::string GetTypeString() const override { return std::format("CharLiteral(\"{}\")", value); }
};

struct BoolLiteral final : Expression
{
    bool value;
    explicit BoolLiteral(const bool val, const SourceLocation loc) : value(val) { this->source_location = loc; }
    [[nodiscard]] std::string GetTypeString() const override { return std::format("BoolLiteral({})", value); }
};

struct BinaryExpression final : Expression
{
    Token operator_token;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpression(Token token, std::unique_ptr<Expression> left_node, std::unique_ptr<Expression> right_node, const SourceLocation loc)
        : operator_token(std::move(token)),
          left(std::move(left_node)),
          right(std::move(right_node))
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Binary Expression, operator({}), left: {}, right: {}", operator_token.type, left.get(), right.get());
    }
};

struct UnaryExpression final : Expression
{
    Token operator_token;
    std::unique_ptr<Expression> right;

    UnaryExpression(Token token, std::unique_ptr<Expression> right_node, const SourceLocation loc)
        : operator_token(std::move(token)),
          right(std::move(right_node))
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Unary Expression, operator({}), right: {}", operator_token.type, right.get());
    }
};

struct CallExpression final : Expression
{
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    bool is_constructor_call = false;

    CallExpression(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args, const SourceLocation loc)
        : callee(std::move(callee)),
          arguments(std::move(args))
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        std::string args_str;
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            args_str += arguments[i] ? arguments[i]->GetTypeString() : "nullptr";
            if(i + 1 < arguments.size()) args_str += ", ";
        }
        return std::format("CallExpression(callee: {}, args: [{}])", callee->GetTypeString(), args_str);
    }
};

struct IdentifierExpression final : Expression
{
    std::string name;
    explicit IdentifierExpression(std::string name, const SourceLocation loc) : name(std::move(name)) { this->source_location = loc; }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Identifier, name(\"{}\")", name);
    }
};

struct AssignmentExpression final : Expression
{
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
    AssignmentExpression(std::unique_ptr<Expression> target, std::unique_ptr<Expression> val, const SourceLocation loc)
        : target(std::move(target)), value(std::move(val))
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        if(const auto* id_expr = dynamic_cast<const IdentifierExpression*>(target.get()))
        {
            return std::format("AssignmentExpression(name: \"{}\", value: {})", id_expr->name, value->GetTypeString());
        }
        return std::format("AssignmentExpression(target: {}, value: {})", target->GetTypeString(), value->GetTypeString());
    }
};

struct ArrayLiteral final : Expression
{
    std::vector<std::unique_ptr<Expression>> elements;

    ArrayLiteral(std::vector<std::unique_ptr<Expression>> elements, const SourceLocation loc)
        : elements(std::move(elements)) 
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("ArrayLiteral({} elements)", elements.size());
    }
};

struct IndexAccess final : Expression
{
    std::unique_ptr<Expression> array_expr;
    std::unique_ptr<Expression> index_expr;

    IndexAccess(std::unique_ptr<Expression> array, std::unique_ptr<Expression> index, const SourceLocation loc)
        : array_expr(std::move(array)), index_expr(std::move(index)) 
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("IndexAccess(array: {}, index: {})", array_expr->GetTypeString(), index_expr->GetTypeString());
    }
};

struct PropertyAccess final : Expression
{
    std::unique_ptr<Expression> object_expr;
    std::string property_name;
    std::optional<int32_t> cached_enum_value; // tried without this at first, but compiler doesnt have clear way to access the value

    PropertyAccess(
        std::unique_ptr<Expression> obj, std::string prop,
        const SourceLocation loc)
        : object_expr(std::move(obj)), property_name(std::move(prop))
    {
        this->source_location = loc;
    }

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("PropertyAccess(object: {}, property: \"{}\")", object_expr->GetTypeString(), property_name);
    }
};
