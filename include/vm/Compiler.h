#pragma once
#include <cassert>
#include <filesystem>

#include "frontend/Statement.h"
#include "vm/Chunk.h"
#include <memory>
#include <vector>

#include "frontend/ProjectConfig.h"

class Compiler
{
public:
    explicit Compiler(ProjectConfig* project_config) : project_config{project_config} {}

    std::unique_ptr<Chunk> Compile(const std::vector<std::unique_ptr<ASTNode>>& statements);

    std::unordered_map<std::string, uint16_t> global_offsets;
    uint16_t global_bytes_count = 0;

private:
    void CompileStatement(const Statement* statement);
    void CompileVariableDeclaration(const VariableDeclaration* variable_declaration);
    void CompileFunctionDeclaration(const FunctionDeclaration* function_declaration);
    void CompileReturnStatement(const ReturnStatement* return_statement);
    void CompileBodyStatement(const BodyStatement* body_statement);
    void CompileIfStatement(const IfStatement* if_statement);
    void CompileWhileStatement(const WhileStatement* while_statement);
    void CompileExpressionStatement(const ExpressionStatement* expression_statement);
    void CompileContinueStatement(const ContinueStatement* continue_statement);
    void CompileBreakStatement(const BreakStatement* break_statement);
    void CompileNativeModuleStatement(const NativeModuleStatement* mod_stmt);
    void CompileNativeFunctionDeclaration(const NativeFunctionDeclaration* native_function_declaration);
    void CompileForLoop(const ForLoop* for_loop);


    void CompileExpression(const Expression* expression);
    void CompileLiteral(const ConstantValue& value, uint32_t line) const;
    void CompileLogicalAnd(const BinaryExpression* binary_expression);
    void CompileLogicalOr(const BinaryExpression* binary_expression);
    void CompileBinaryExpression(const BinaryExpression* binary_expression);
    void CompileUnaryExpression(const UnaryExpression* unary_expression);
    void CompileIdentifierExpression(const IdentifierExpression* identifier_expression) const;
    void CompileCallExpression(const CallExpression* call_expression);
    void CompileArrayAssignmentExpression(
            const AssignmentExpression* assignment_expression, uint32_t line, const IndexAccess* index_access);
    void CompileVariableAssignmentExpression(
            const AssignmentExpression* assignment_expression, uint32_t line, const IdentifierExpression* identifier);
    void CompilePropertyAssignmentExpression(const AssignmentExpression* assignment_expression, uint32_t line, const PropertyAccess* property_access);
    void CompileAssignmentExpression(const AssignmentExpression* assignment_expression);
    void CompileArrayLiteral(const ArrayLiteral* array_literal);
    void CompileIndexAccess(const IndexAccess* index_access);
    void CompilePropertyAccess(const PropertyAccess* property_access);
    void CompileSwitchExpression(const SwitchExpression* switch_expression);


    void ForLoopIterableStruct(const ForLoop* for_loop, uint32_t line, const StructType* struct_type);
    void ForLoopArray(const ForLoop* for_loop, uint32_t line, const ArrayType* array);
    void ForLoopString(const ForLoop* for_loop, uint32_t line, PrimitiveType* get);


    int64_t GetLocalVariableIndex(const std::string& name) const;

    struct Local {
        std::string name;
        const Type* type;
    };

    std::unique_ptr<Chunk> current_chunk;
    size_t scope_depth{0};
    std::vector<Local> locals;

    std::vector<size_t> break_placeholders; // when a break is met, it doesnt know where the body ends, need to update when reached
    std::vector<size_t> loop_starts; // when a continue is met, it doesnt know where the loop starts
    std::vector<size_t> loop_local_counts; // tracks number of locals at the start of each loop to properly pop on break/continue

    std::filesystem::path current_native_module_path{};
    ProjectConfig* project_config; // non owning

};
