#pragma once
#include <memory>
#include <numeric>

class Type;

#include "frontend/Expression.h"
#include "vm/ConstantValue.h"

// Instruction that does something
// can be like a variable declaration,
// or x = x + 1;
// return statement.. while statement...
struct Statement : ASTNode
{};

struct ExportableStatement : Statement
{
    bool is_exported = false;
};


struct VariableDeclaration final : ExportableStatement
{
    Token name;
    std::optional<Token> type;
    std::unique_ptr<Expression> initializer;
    const Type* type_info = nullptr;

    [[nodiscard]] std::string GetTypeString() const override
    {
        std::string type_str = type ? type->lexeme : "null";
        if(initializer)
        {
            return std::format(R"(VariableDeclaration(name: "{}", type: "{}", initializer: {}))", name.lexeme, type_str, initializer->GetTypeString());
        }
        return std::format(R"(VariableDeclaration(name: "{}", type: "{}", initializer: nullptr))", name.lexeme, type_str);
    }

    VariableDeclaration(
        Token name,
        std::optional<Token> type,
        std::unique_ptr<Expression> initializer,
        const SourceLocation& loc
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

    explicit ReturnStatement(std::unique_ptr<Expression> value, const SourceLocation& loc) : value{std::move(value)}
    { this->source_location = loc; }
};

struct BodyStatement final : Statement
{
    std::vector<std::unique_ptr<ASTNode>> statements;

    [[nodiscard]] std::string GetTypeString() const override
    {
        if(statements.empty()) return "BodyStatement(empty)";

        std::string inner;
        for(size_t i = 0; i < statements.size(); ++i)
        {
            inner += statements[i] ? statements[i]->GetTypeString() : "nullptr";
            if(i + 1 < statements.size())
            {
                inner += ", ";
            }
        }
        return std::format("BodyStatement([{}])", inner);
    }

    explicit BodyStatement(const SourceLocation& loc) { this->source_location = loc; }
};

struct WhileStatement final : Statement
{
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BodyStatement> body;
    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("WhileStatement(condition: {}, body: {})", condition.get(), body.get());
    }

    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<BodyStatement> body, const SourceLocation& loc) :
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
        if(else_branch)
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
        const SourceLocation& loc)
    : condition{std::move(condition)}, body{std::move(body)}, else_branch{std::move(else_branch)}
    { this->source_location = loc; }
};

struct Parameter
{
    Token name;
    Token type_name;
    const Type* type_info = nullptr;
};


struct MethodSignature
{
    Token name;
    std::vector<Parameter> parameters;
    std::optional<Token> return_type;
    const Type* return_type_info = nullptr;
};

struct FunctionDeclaration final : ExportableStatement
{
    MethodSignature method_signature;
    std::optional<Parameter> receiver; // the self class for methods
    std::unique_ptr<BodyStatement> body;

    FunctionDeclaration(
        MethodSignature method_signature, std::unique_ptr<BodyStatement> body_node,
        std::optional<Parameter> receiver, const SourceLocation& loc)
        : method_signature{std::move(method_signature)},
          receiver(std::move(receiver)),
          body(std::move(body_node))
    { this->source_location = loc; }

    [[nodiscard]] std::string GetTypeString() const override
    {
        std::string params_str;
        for(size_t i = 0; i < method_signature.parameters.size(); ++i)
        {
            params_str += std::format("{}: {}", method_signature.parameters[i].name.lexeme, method_signature.parameters[i].type_name.lexeme);
            if(i + 1 < method_signature.parameters.size()) params_str += ", ";
        }
        std::string ret_type = method_signature.return_type ? method_signature.return_type->lexeme : "void";
        return std::format(R"(FunctionDeclaration(name: "{}", params: [{}], return: "{}", body: {}))",
                           method_signature.name.lexeme, params_str, ret_type, body ? body->GetTypeString() : "nullptr");
    }
};

struct ExpressionStatement final : Statement
{
    std::unique_ptr<Expression> expression;
    ExpressionStatement(std::unique_ptr<Expression> expr, const SourceLocation& loc)
        : expression(std::move(expr)) { this->source_location = loc; }

    [[nodiscard]] std::string GetTypeString() const override {
        return std::format("ExpressionStatement(expr: {})", expression.get());
    }
};

struct BreakStatement final : Statement
{
    [[nodiscard]] std::string GetTypeString() const override
    {
        return "BreakStatement";
    }

    explicit BreakStatement(const SourceLocation& loc) { this->source_location = loc; }
};

struct ContinueStatement final : Statement
{
    [[nodiscard]] std::string GetTypeString() const override
    {
        return "ContinueStatement";
    }

    explicit ContinueStatement(const SourceLocation& loc) { this->source_location = loc; }
};


struct NativeImportStatement final : Statement
{
    Token name;
    [[nodiscard]] std::string GetTypeString() const override {
        return std::format("NativeImportStatement({})", name.lexeme);
    }

    explicit NativeImportStatement(Token name, const SourceLocation& loc) : name(std::move(name))
    {
        this->source_location = loc;
    }
};

struct NativeFunctionDeclaration final : Statement
{
    MethodSignature method_signature;
    std::string original_name;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return "NativeFunctionDeclaration";
    }

    NativeFunctionDeclaration(MethodSignature method_signature, const SourceLocation& loc) :
    method_signature{std::move(method_signature)}, original_name{this->method_signature.name.lexeme}
    {
        this->source_location = loc;
    }
};

struct StructDeclaration final : ExportableStatement
{
    Token name;
    std::vector<std::pair<Token, Token>> fields; // {"x", "int"}, {"y", "int"} ...

    [[nodiscard]] std::string GetTypeString() const override
    {
        return "StructDeclaration";
    }

    StructDeclaration(Token name, std::vector<std::pair<Token, Token>> fields, const SourceLocation& loc) :
    name(std::move(name)), fields(std::move(fields))
    {
        this->source_location = loc;
    }
};

struct EnumVariant
{
    Token name;
    std::optional<int32_t> value; // nullptr if they didn't specify '= X', so it has to be a pointer
};


struct EnumDeclaration final : ExportableStatement
{
    Token name;
    std::vector<EnumVariant> variant_names;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return "EnumDeclaration";
    }

    EnumDeclaration(Token name, std::vector<EnumVariant> variant_names, const SourceLocation& loc) :
    name(std::move(name)), variant_names(std::move(variant_names))
    {
        this->source_location = loc;
    }
};

struct InterfaceDeclaration final : ExportableStatement
{
    Token name;
    std::vector<MethodSignature> methods;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return "InterfaceDeclaration";
    }

    InterfaceDeclaration(Token name, std::vector<MethodSignature> methods, const SourceLocation& loc) :
    name(std::move(name)), methods(std::move(methods))
    {
        this->source_location = loc;
    }
};


struct ForLoop final : Statement
{
    Token iterator_name;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<BodyStatement> body;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("ForLoop(iterator: '{}', iterable: {}, body: {})",
            iterator_name.lexeme,
            iterable ? iterable->GetTypeString() : "null",
            body ? body->GetTypeString() : "null");
    }

    ForLoop(
        Token iterator_name,
        std::unique_ptr<Expression> iterable,
        std::unique_ptr<BodyStatement> body,
        const SourceLocation& loc
    ) :
        iterator_name(std::move(iterator_name)),
        iterable{std::move(iterable)},
        body{std::move(body)}
    {
        this->source_location = loc;
    }
};


struct ExtendStatement final : Statement
{
    Token target_struct;
    Token interface_extending;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format(
            "ExtendStatement(target: '{}', interface: '{}')",
            target_struct.lexeme,
            interface_extending.lexeme
        );
    }

    ExtendStatement(Token target_struct, Token interface_extending, SourceLocation loc)
    : target_struct{std::move(target_struct)}, interface_extending{std::move(interface_extending)}
    {
        this->source_location = loc;
    }
};

struct ImportStatement final : Statement
{
    std::string module_name;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Import({})", module_name);
    }

    ImportStatement(std::string module_name, const SourceLocation& loc)
    : module_name{std::move(module_name)}
    {
        this->source_location = loc;
    }
};

struct ModuleDeclaration final : Statement
{
    std::string name;

    [[nodiscard]] std::string GetTypeString() const override
    {
        return std::format("Module({})", name);
    }

    ModuleDeclaration(std::string name, const SourceLocation& loc)
    : name{std::move(name)}
    {
        this->source_location = loc;
    }

};