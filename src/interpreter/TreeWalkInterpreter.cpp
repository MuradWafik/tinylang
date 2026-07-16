#include "interpreter/TreeWalkInterpreter.h"

#include <ranges>

// NOLINTBEGIN(misc-no-recursion)
RuntimeValue TreeWalkInterpreter::Evaluate(Expression* expr)
{
    if(const auto* int_literal = dynamic_cast<IntegerLiteral*>(expr)) return int_literal->value;
    if(const auto* float_literal = dynamic_cast<FloatLiteral*>(expr)) return float_literal->value;
    if(const auto* bool_literal = dynamic_cast<BoolLiteral*>(expr)) return bool_literal->value;
    if(const auto* string_literal = dynamic_cast<StringLiteral*>(expr)) return string_literal->value;
    if(const auto* binary_expr = dynamic_cast<BinaryExpression*>(expr)) return EvaluateBinaryExpression(binary_expr);
    if(const auto* unary_expr = dynamic_cast<UnaryExpression*>(expr)) return EvaluateUnaryExpression(unary_expr);
    if(const auto* identifier_expr = dynamic_cast<IdentifierExpression*>(expr)) return EvaluateIdentifierExpression(identifier_expr);
    if(const auto* assignment_expr = dynamic_cast<AssignmentExpression*>(expr)) return EvaluateAssignmentExpression(assignment_expr);
    if(const auto* call_expr = dynamic_cast<CallExpression*>(expr)) return EvaluateCallExpression(call_expr);

    return std::monostate{};
}

RuntimeValue TreeWalkInterpreter::EvaluateBinaryExpression(const BinaryExpression* binary_expression)
{
    const auto left = Evaluate(binary_expression->left.get());
    const auto right = Evaluate(binary_expression->right.get());
    const auto op = binary_expression->operator_token.type;

    return std::visit(overloaded {
        [&](const int l, const int r) -> RuntimeValue {
            switch (op) {
                case TokenType::Plus:         return l + r;
                case TokenType::Minus:        return l - r;
                case TokenType::Star:         return l * r;
                case TokenType::Slash:        return l / r;
                case TokenType::Less:         return l < r;
                case TokenType::LessEqual:    return l <= r;
                case TokenType::Greater:      return l > r;
                case TokenType::GreaterEqual: return l >= r;
                case TokenType::Equal:        return l == r;
                case TokenType::NotEqual:     return l != r;
                default:                      return std::monostate{};
            }
        },

        [&](const float l, const float r) -> RuntimeValue {
            switch (op) {
                case TokenType::Plus:         return l + r;
                case TokenType::Minus:        return l - r;
                case TokenType::Star:         return l * r;
                case TokenType::Slash:        return l / r;
                case TokenType::Less:         return l < r;
                case TokenType::LessEqual:    return l <= r;
                case TokenType::Greater:      return l > r;
                case TokenType::GreaterEqual: return l >= r;
                case TokenType::Equal:        return l == r;
                case TokenType::NotEqual:     return l != r;
                default:                      return std::monostate{};
            }
        },

        [&](const std::string& l, const std::string& r) -> RuntimeValue {
            if (op == TokenType::Plus)       return l + r;
            if (op == TokenType::Equal)      return l == r;
            if (op == TokenType::NotEqual)   return l != r;
            return std::monostate{};
        },

        [&](const bool l, const bool r) -> RuntimeValue {
            if (op == TokenType::AndAnd)     return l && r;
            if (op == TokenType::OrOr)       return l || r;
            if (op == TokenType::Equal)      return l == r;
            if (op == TokenType::NotEqual)   return l != r;
            return std::monostate{};
        },
        // should never reach here as the semantic analyzer would have noticed a fault already
        [](const auto& l, const auto& r) -> RuntimeValue {
            return std::monostate{};
        }
    }, left, right);
}

RuntimeValue TreeWalkInterpreter::EvaluateUnaryExpression(const UnaryExpression* unary_expression)
{
    const auto op = unary_expression->operator_token.type;
    const auto operand_val = Evaluate(unary_expression->right.get());

    return std::visit(overloaded {
        [&](const int val) -> RuntimeValue {
            if (op == TokenType::Minus) return -val;
            return std::monostate{};
        },
        [&](const float val) -> RuntimeValue {
            if (op == TokenType::Minus) return -val;
            return std::monostate{};
        },
        [&](const bool val) -> RuntimeValue {
            if (op == TokenType::Negate) return !val;
            return std::monostate{};
        },
        [](const auto& val) -> RuntimeValue {
            return std::monostate{};
        }
    },
    operand_val);
}

RuntimeValue TreeWalkInterpreter::EvaluateIdentifierExpression(const IdentifierExpression* identifier_expression) const
{
    return environment->Get(identifier_expression->name);
}

RuntimeValue TreeWalkInterpreter::EvaluateAssignmentExpression(const AssignmentExpression* assignment_expression)
{
    const auto value = Evaluate(assignment_expression->value.get());
    environment->Assign(assignment_expression->name, value);
    return std::monostate{}; // once again not sure, return the value or ignore?
}

RuntimeValue TreeWalkInterpreter::EvaluateCallExpression(const CallExpression* call_expression)
{
    const auto function = environment->Get(call_expression->function_name);
    std::vector<RuntimeValue> arguments{};
    for(const auto& argument: call_expression->arguments)
    {
        arguments.push_back(Evaluate(argument.get()));
    }

    if(std::holds_alternative<std::shared_ptr<NativeFunctionWrapper>>(function))
    {
        const auto native_func = std::get<std::shared_ptr<NativeFunctionWrapper>>(function);
        return_value = native_func->func(arguments);
    }
    else
    {
        // functions have their own scope, can only reference global variables and their own
        const auto old_env = environment;
        const auto function_env = std::make_shared<Environment>(global_environment);

        // DEFINE ARGUMENTS IN THE NEW ENVIRONMENT!
        const auto* func_decl = std::get<const FunctionDeclaration*>(function);
        for (size_t i = 0; i < arguments.size(); ++i) {
            function_env->Define(func_decl->parameters[i].name, arguments[i]);
        }

        environment = function_env;
        Execute(func_decl->body.get());
        environment = old_env;
    }

    // the return value gets set to whatever the function executed body returns, even if void
    RuntimeValue final_return_value = return_value;
    is_returning = false; // RESET THE FLAG!
    
    return final_return_value;

}

void TreeWalkInterpreter::Execute(Statement* statement)
{
    if(const auto* expression_stmt = dynamic_cast<ExpressionStatement*>(statement)) return ExecuteExpressionStatement(expression_stmt);
    if(const auto* variable_decl = dynamic_cast<VariableDeclaration*>(statement)) return ExecuteVariableDeclaration(variable_decl);
    if(const auto* body_stmt = dynamic_cast<BodyStatement*>(statement)) return ExecuteBodyStatement(body_stmt);
    if(const auto* if_stmt = dynamic_cast<IfStatement*>(statement)) return ExecuteIfStatement(if_stmt);
    if(const auto* while_stmt = dynamic_cast<WhileStatement*>(statement)) return ExecuteWhileStatement(while_stmt);
    if(const auto* break_stmt = dynamic_cast<BreakStatement*>(statement)) return ExecuteBreakStatement(break_stmt);
    if(const auto* continue_stmt = dynamic_cast<ContinueStatement*>(statement)) return ExecuteContinueStatement(continue_stmt);
    if(const auto* return_stmt = dynamic_cast<ReturnStatement*>(statement)) return ExecuteReturnStatement(return_stmt);
    if(const auto* fn_decl = dynamic_cast<FunctionDeclaration*>(statement)) return ExecuteFunctionDeclaration(fn_decl);
}

void TreeWalkInterpreter::ExecuteExpressionStatement(const ExpressionStatement* expression_statement)
{
     Evaluate(expression_statement->expression.get());
}

void TreeWalkInterpreter::ExecuteVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    const auto initializer = Evaluate(variable_declaration->initializer.get());
    environment->Define(variable_declaration->name, initializer);
}

void TreeWalkInterpreter::ExecuteBodyStatement(const BodyStatement* body_statement)
{
    const auto old_env = environment;
    environment = std::make_shared<Environment>(old_env);

    for(auto& node : body_statement->statements)
    {

        if(auto* stmt = dynamic_cast<Statement*>(node.get())) Execute(stmt);
        else if(auto* expr = dynamic_cast<Expression*>(node.get())) Evaluate(expr);

        if (is_breaking || is_continuing || is_returning) {
            break; // Bubble the signal up the call stack
        }
    }
    environment = old_env;
}

void TreeWalkInterpreter::ExecuteIfStatement(const IfStatement* if_statement)
{
    // Garunteed to be boolean from semantic analysis
    const bool is_true = std::get<bool>(Evaluate(if_statement->condition.get()));

    if(is_true)
    {
        return ExecuteBodyStatement(if_statement->body.get());
    }

    // Else branch will just branch through else ifs, finalising at the else
    if(if_statement->else_branch != nullptr)
    {
        return Execute(if_statement->else_branch.get());
    }
}

void TreeWalkInterpreter::ExecuteWhileStatement(const WhileStatement* while_statement)
{
    while (std::get<bool>(Evaluate(while_statement->condition.get()))) {
        Execute(while_statement->body.get());

        if (is_returning)
        {
            return;
        }

        if (is_breaking) {
            is_breaking = false; // Reset flag, but break -- same logic in here as interpreter
            break;
        }

        if (is_continuing) {
            is_continuing = false; // Reset flag and continue to next iteration
        }
    }
}

void TreeWalkInterpreter::ExecuteBreakStatement(const BreakStatement*)
{
    is_breaking = true;
}

void TreeWalkInterpreter::ExecuteContinueStatement(const ContinueStatement*)
{
    is_continuing = true;
}

void TreeWalkInterpreter::ExecuteReturnStatement(const ReturnStatement* return_statement)
{
    auto val = Evaluate(return_statement->value.get());
    is_returning = true;
    return_value = val;
}

void TreeWalkInterpreter::ExecuteFunctionDeclaration(const FunctionDeclaration* function_declaration)
{
    environment->Define(function_declaration->name, function_declaration);
}

// NOLINTEND(misc-no-recursion)
