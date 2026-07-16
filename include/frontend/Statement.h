#pragma once
#include <memory>
#include <numeric>

#include "frontend/Expression.h"
#include "interpreter/RuntimeValue.h"

// Instruction that does something
// can be like a variable declaration,
// or x = x + 1;
// return statement.. while statement...
struct Statement : ASTNode
{};

struct VariableDeclaration final : Statement
{
    std::string name;
    std::string type;
    std::unique_ptr<Expression> initializer;
    [[nodiscard]] std::string GetTypeString() const override
    {
        if (initializer) {
            return std::format(R"(VariableDeclaration(name: "{}", type: "{}", initializer: {}))", name, type, initializer.get());
        }
        return std::format(R"(VariableDeclaration(name: "{}", type: "{}", initializer: nullptr))", name, type);
    }

    VariableDeclaration(
        std::string name,
        std::string type,
        std::unique_ptr<Expression> initializer,
        SourceLocation loc
        ) :
    name(std::move(name)),
    type(std::move(type)),
    initializer{std::move(initializer)}
    { this->source_location = loc; }
};

struct ReturnStatement final : Statement
{
    std::unique_ptr<Expression> value;
    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("ReturnStatement(value: {})", value ? value->GetTypeString() : "void");
    }

    [[nodiscard]] bool IsVoidReturn() const { return value == nullptr; }

    explicit ReturnStatement(std::unique_ptr<Expression> value, SourceLocation loc) : value{std::move(value)}
    { this->source_location = loc; }
};

struct BodyStatement final : Statement
{
    std::vector<std::unique_ptr<ASTNode>> statements;

    [[nodiscard]] std::string GetTypeString() const override
    {
        if (statements.empty()) return "BodyStatement(empty)";

        std::string inner;
        for (size_t i = 0; i < statements.size(); ++i)
        {
            inner += statements[i] ? statements[i]->GetTypeString() : "nullptr";
            if (i + 1 < statements.size())
            {
                inner += ", ";
            }
        }

        return std::format("BodyStatement([{}])", inner);
    }

    explicit BodyStatement(SourceLocation loc) { this->source_location = loc; }
};

struct WhileStatement final : Statement
{
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BodyStatement> body;
    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("WhileStatement(condition: {}, body: {})", condition.get(), body.get());
    }

    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<BodyStatement> body, SourceLocation loc) :
        condition{std::move(condition)}, body{std::move(body)}
    { this->source_location = loc; }
};

struct IfStatement final : Statement
{
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BodyStatement> body;
    std::unique_ptr<Statement> else_branch; // else if is then just a sub if branch in the else

    [[nodiscard]] std::string GetTypeString() const override
    {
        if (else_branch)
        {
            return std::format("IfStatement(condition: {}, then: {}, else: {})",
                               condition.get(), body.get(), else_branch.get());
        }
        return std::format("IfStatement(condition: {}, body: {})", condition.get(), body.get());
    }

    IfStatement(
        std::unique_ptr<Expression> condition,
        std::unique_ptr<BodyStatement> body,
        std::unique_ptr<Statement> else_branch,
        SourceLocation loc)
    : condition{std::move(condition)}, body{std::move(body)}, else_branch{std::move(else_branch)}
    { this->source_location = loc; }
};

struct Parameter
{
    std::string name;
    std::string type_name;
};

struct FunctionDeclaration final : Statement
{
    std::string name;
    std::vector<Parameter> parameters;
    std::string return_type;
    std::unique_ptr<BodyStatement> body;

    FunctionDeclaration(std::string name,
                        std::vector<Parameter> params,
                        std::string ret_type,
                        std::unique_ptr<BodyStatement> body_node,
                        SourceLocation loc)
        : name(std::move(name)),
          parameters(std::move(params)),
          return_type(std::move(ret_type)),
          body(std::move(body_node))
    { this->source_location = loc; }

    [[nodiscard]] std::string GetTypeString() const override
    {
        std::string params_str;
        for (size_t i = 0; i < parameters.size(); ++i) {
            params_str += std::format("{}: {}", parameters[i].name, parameters[i].type_name);
            if (i + 1 < parameters.size()) params_str += ", ";
        }
        return std::format(R"(FunctionDeclaration(name: "{}", params: [{}], return: "{}", body: {}))",
                           name, params_str, return_type, body.get());
    }
};

struct ExpressionStatement final : Statement
{
    std::unique_ptr<Expression> expression;
    explicit ExpressionStatement(std::unique_ptr<Expression>&& expr, SourceLocation loc)
        : expression(std::move(expr)) { this->source_location = loc; }

    [[nodiscard]] std::string GetTypeString() const override {
        return std::format("ExpressionStatement(expr: {})", expression.get());
    }
};

struct BreakStatement final : Statement
{
    [[nodiscard]] std::string GetTypeString() const override {
        return "BreakStatement";
    }

    explicit BreakStatement(SourceLocation loc) { this->source_location = loc; }
};

struct ContinueStatement final : Statement
{
    [[nodiscard]] std::string GetTypeString() const override {
        return "ContinueStatement";
    }

    explicit ContinueStatement(SourceLocation loc) { this->source_location = loc; }
};
