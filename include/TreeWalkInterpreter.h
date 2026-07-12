#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <variant>
#include <variant>
#include <variant>
#include <variant>

#include "Environment.h"
#include "RuntimeValue.h"
#include "SemanticAnalyzer.h"
#include "Statement.h"



class TreeWalkInterpreter {
public:
    explicit TreeWalkInterpreter(const SemanticAnalyzer& semantic_analyzer)
        : semantic_analyzer{semantic_analyzer}
    {}

    void Execute(Statement* statement);
    RuntimeValue Evaluate(Expression* expr);

private:
    std::unordered_map<std::string, RuntimeValue> runtime_values;
    const SemanticAnalyzer& semantic_analyzer;
    std::shared_ptr<Environment> global_environment = std::make_shared<Environment>(nullptr);
    std::shared_ptr<Environment> environment = global_environment; // starter environment is the global environment


    RuntimeValue EvaluateBinaryExpression(const BinaryExpression* binary_expression);
    RuntimeValue EvaluateUnaryExpression(const UnaryExpression* unary_expression);
    RuntimeValue EvaluateIdentifierExpression(const IdentifierExpression* identifier_expression) const;
    RuntimeValue EvaluateAssignmentExpression(const AssignmentExpression* assignment_expression);
    RuntimeValue EvaluateCallExpression(const CallExpression* call_expression);


    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

};
