#pragma once
#include <cassert>

#include "frontend/Statement.h"
#include "vm/Chunk.h"
#include <memory>
#include <vector>

class Compiler {
public:
    Compiler() = default;

    std::unique_ptr<Chunk> Compile(const std::vector<std::unique_ptr<ASTNode>>& statements);

private:
    void CompileStatement(const Statement* statement);
    void CompileVariableDeclaration(const VariableDeclaration* variable_declaration);
    void CompileFunctionDeclaration(const FunctionDeclaration* function_declaration);
    void CompileReturnStatement(const ReturnStatement* return_statement);
    void CompileBodyStatement(const BodyStatement* body_statement);
    void CompileIfStatement(const IfStatement* if_statement);
    void CompileWhileStatement(const WhileStatement* while_statement);
    void CompileExpressionStatement(const ExpressionStatement* expression_statement);
    void CompileContinueStatement(const ContinueStatement* continue_statement) const;
    void CompileBreakStatement(const BreakStatement* break_statement);




    void CompileExpression(const Expression* expression);
    void CompileLiteral(const RuntimeValue& value, uint32_t line) const;

    void CompileLogicalAnd(const BinaryExpression* binary_expression);
    void CompileLogicalOr(const BinaryExpression* binary_expression);

    void CompileBinaryExpression(const BinaryExpression* binary_expression);
    void CompileUnaryExpression(const UnaryExpression* unary_expression);
    void CompileIdentifierExpression(const IdentifierExpression* identifier_expression);
    void CompileCallExpression(const CallExpression* call_expression);
    void CompileAssignmentExpression(const AssignmentExpression* assignment_expression);

    int64_t GetLocalVariableIndex(const std::string& name);

    std::unique_ptr<Chunk> current_chunk;
    size_t scope_depth{0};
    std::vector<std::string> locals;
    std::vector<size_t> break_placeholders; // when a break is met, it doesnt know where the body ends, need to update when reached
    std::vector<size_t> loop_starts; // when a continue is met, it doesnt know where the loop starts

};
