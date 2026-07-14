#pragma once
#include <string>
#include <unordered_map>
#include <variant>

#include "Environment.h"
#include "NativeFunction.h"
#include "RuntimeValue.h"
#include "SemanticAnalyzer.h"
#include "Statement.h"

// Used for the visitor for binary and unary operations
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


class TreeWalkInterpreter {
public:
    explicit TreeWalkInterpreter(const SemanticAnalyzer& semantic_analyzer)
        : semantic_analyzer{semantic_analyzer}
    {
        NativeFunction::RegisterImplementations(environment.get());
    }

    void Execute(Statement* statement);
    RuntimeValue Evaluate(Expression* expr);

private:
    std::unordered_map<std::string, RuntimeValue> runtime_values;
    const SemanticAnalyzer& semantic_analyzer;
    std::shared_ptr<Environment> global_environment = std::make_shared<Environment>(nullptr);
    std::shared_ptr<Environment> environment = global_environment; // starter environment is the global environment

    bool is_returning = false;
    bool is_breaking = false;
    bool is_continuing = false;
    RuntimeValue return_value;


    RuntimeValue EvaluateBinaryExpression(const BinaryExpression* binary_expression);
    RuntimeValue EvaluateUnaryExpression(const UnaryExpression* unary_expression);
    RuntimeValue EvaluateIdentifierExpression(const IdentifierExpression* identifier_expression) const;
    RuntimeValue EvaluateAssignmentExpression(const AssignmentExpression* assignment_expression);
    RuntimeValue EvaluateCallExpression(const CallExpression* call_expression);

    void ExecuteExpressionStatement(const ExpressionStatement* expression_statement);
    void ExecuteVariableDeclaration(const VariableDeclaration* variable_declaration);
    void ExecuteBodyStatement(const BodyStatement* body_statement);
    void ExecuteIfStatement(const IfStatement* if_statement);
    void ExecuteWhileStatement(const WhileStatement* while_statement);
    void ExecuteBreakStatement(const BreakStatement*);
    void ExecuteContinueStatement(const ContinueStatement*);
    void ExecuteReturnStatement(const ReturnStatement* return_statement);
    void ExecuteFunctionDeclaration(const FunctionDeclaration* function_declaration);
};
