#include "TreeWalkInterpreter.h"

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
    auto function = environment.get()->Get(call_expression->function_name);
    std::vector<RuntimeValue> arguments{};
    for(const auto& argument: call_expression->arguments)
    {
        arguments.push_back(Evaluate(argument.get()));
    }
    Environment function_environment{environment.get()};

}



