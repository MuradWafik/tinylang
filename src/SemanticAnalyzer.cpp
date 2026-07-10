#include "SemanticAnalyzer.h"

#include <ranges>

#include "Expression.h"
#include "Statement.h"
#include "Type.h"


std::expected<void, std::string> SemanticAnalyzer::Analyze(std::vector<std::unique_ptr<ASTNode>>& program)
{
    // Initialize global scope
    symbol_table.PushScope();
    InitializeDefaults();

    for (auto& node : program)
    {
        if (auto result = AnalyzeNode(node.get()); !result)
        {
            return std::unexpected(result.error());
        }
    }
    symbol_table.PopScope();
    return {};
}


std::expected<void, std::string> SemanticAnalyzer::AnalyzeNode(ASTNode* node)
{
    // let statements and expressions handle their own dispatching
    if (auto* stmt = dynamic_cast<Statement*>(node))
    {
        return AnalyzeStatement(stmt);
    }
    if(auto* expr = dynamic_cast<Expression*>(node))
    {
        if (auto res = AnalyzeExpression(expr); !res) return std::unexpected(res.error());
        return {};
    }
    return std::unexpected("Unknown node type");

}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeStatement(Statement* stmt) {
    if(const auto* var_decl = dynamic_cast<VariableDeclaration*>(stmt)) return AnalyzeVariableDeclaration(var_decl);
    if(const auto* if_stmt = dynamic_cast<IfStatement*>(stmt)) return AnalyzeIfStatement(if_stmt);
    if(const auto* while_stmt = dynamic_cast<WhileStatement*>(stmt)) return AnalyzeWhileStatement(while_stmt);

    return std::unexpected("Unknown statement type");
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeExpression(Expression* expr) {
    if(auto* binary_expr = dynamic_cast<BinaryExpression*>(expr)) return AnalyzeBinaryExpression(binary_expr);
    if(auto* id_expr = dynamic_cast<IdentifierExpression*>(expr)) {
        // return AnalyzeIdentifierExpression(id_expr);
    }
    if(auto* unary_expr = dynamic_cast<UnaryExpression*>(expr)) return AnalyzeUnaryExpression(unary_expr);
    if(auto* bool_node = dynamic_cast<BoolLiteral*>(expr))
    {
        bool_node->type_info = PrimitiveType::Bool.get();
        return PrimitiveType::Bool.get(); // Literals return their type
    }
    if(auto* int_node = dynamic_cast<IntegerLiteral*>(expr))
    {
        int_node->type_info = PrimitiveType::Int.get();
        return PrimitiveType::Int.get();
    }
    if(auto* float_node = dynamic_cast<FloatLiteral*>(expr))
    {
        float_node->type_info = PrimitiveType::Float.get();
        return PrimitiveType::Float.get();
    }
    if(auto* string_node = dynamic_cast<StringLiteral*>(expr))
    {
        string_node->type_info = PrimitiveType::String.get();
        return PrimitiveType::String.get();
    }

    return std::unexpected("Unknown expression type");
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    const Type* type = symbol_table.LookupType(variable_declaration->type);
    if(!type)
    {
        return std::unexpected(std::format("Unknown type name {}", variable_declaration->type));
    }

    if(symbol_table.LookupVariable(variable_declaration->name))
    {
        return std::unexpected(std::format("Redefinition of variable {}", variable_declaration->name));
    }

    auto expression_type = AnalyzeExpression(variable_declaration->initializer.get());
    if(!expression_type)
    {
        return std::unexpected(expression_type.error());
    }

    if (!(expression_type.value())->IsAssignableTo(type))
    {
        return std::unexpected(
            std::format("Unable to assign {} to type {}", expression_type.value()->GetName(), type->GetName())
        );
    }
    return {};
}


void SymbolTable::DefineVariable(const Symbol& symbol)
{
    scopes.back().variables[symbol.name] = symbol;
}

std::optional<Symbol> SymbolTable::LookupVariable(const std::string_view name)
{
    for(auto& [variables, types] : std::ranges::reverse_view(scopes))
    {
        if(auto found = variables.find(name); found != variables.end())
        {
            return found->second;
        }
    }
    return std::nullopt;
}

void SymbolTable::DefineType(const std::string_view name, const Type* type)
{
    scopes.back().types[std::string(name)] = type;
}

const Type* SymbolTable::LookupType(const std::string_view name)
{
    for (auto & [variables, types] : std::ranges::reverse_view(scopes))
    {
        if(auto found = types.find(name); found != types.end())
        {
            return found->second;
        }
    }
    return nullptr;
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeIfStatement(const IfStatement* if_statement)
{
    constexpr auto error_string = "Error analyzing if statement:";
    auto bool_condition = AnalyzeExpression(if_statement->condition.get());
    if(!bool_condition)
    {
        return std::unexpected(std::format("{} {}", error_string, bool_condition.error()));
    }

    if(!(bool_condition.value())->IsAssignableTo(PrimitiveType::Bool.get()))
    {
        return std::unexpected(std::format("{} Expected boolean condition in if statement", error_string));
    }

    if(auto body_analysis = AnalyzeNode(if_statement->body.get()); !body_analysis)
    {
        return std::unexpected(std::format("{} {}", error_string, body_analysis.error()));
    }

    if (if_statement->else_branch)
    {
        return AnalyzeNode(if_statement->else_branch.get());
    }
    return {};
}


std::expected<void, std::string> SemanticAnalyzer::AnalyzeWhileStatement(const WhileStatement* while_statement)
{
    constexpr auto error_string = "Error analyzing while statement:";
    auto bool_condition = AnalyzeExpression(while_statement->condition.get());
    if(!bool_condition)
    {
        return std::unexpected(std::format("{} {}", error_string, bool_condition.error()));
    }

    if(!(bool_condition.value())->IsAssignableTo(PrimitiveType::Bool.get()))
    {
        return std::unexpected(std::format("{} Expected boolean condition in a while statement", error_string));
    }

    ++loop_depth;
    if(auto body_analysis = AnalyzeNode(while_statement->body.get()); !body_analysis)
    {
        return std::unexpected(std::format("{} {}", error_string, body_analysis.error()));
    }

    --loop_depth;
    return {};
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeBinaryExpression(BinaryExpression* binary_expression)
{
    const auto left = AnalyzeExpression(binary_expression->left.get());
    if(!left)
    {
        return Return("Error in left side of binary expression");
    }

    const auto right = AnalyzeExpression(binary_expression->right.get());
    if(!right)
    {
        return Return("Error in right side of binary expression");
    }

    // const auto left_type = symbol_table.LookupType(left.value());
    // const auto right_type = symbol_table.LookupType(right.value());
    const auto left_type = left.value();
    const auto right_type = right.value();

    const auto& op = binary_expression->operator_token;
    const auto* return_type = LookupBinaryOperator(op.type, left_type, right_type);
    if(!return_type)
    {
        return Return(
            std::format("Invalid binary expression: cannot apply operator '{}' to types '{}' and '{}'",
                op.lexeme, left_type->GetName(), right_type->GetName())
        );
    }
    binary_expression->type_info = return_type;
    return return_type;
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeUnaryExpression(UnaryExpression* unary_expression)
{
    const auto right = AnalyzeExpression(unary_expression->right.get());
    if(!right)
    {
        return Return("Error in right side of unary expression");
    }
    const auto right_type = right.value();

    const auto& op = unary_expression->operator_token;
    const auto* return_type = LookupUnaryOperator(op.type, right_type);
    if(!return_type)
    {
        return Return(
            std::format("Invalid unary expression: cannot apply operator '{}' to type '{}'",
                op.lexeme, right_type->GetName())
        );
    }
    unary_expression->type_info = return_type;
    return return_type;
}

void SemanticAnalyzer::RegisterBinaryOperator(const TokenType op, const Type* left, const Type* right, const Type* result)
{
    binary_operators[{op, left, right}] = result;
}

const Type* SemanticAnalyzer::LookupBinaryOperator(const TokenType op, const Type* left, const Type* right) const
{
    if (const auto it = binary_operators.find({op, left, right}); it != binary_operators.end())
    {
        return it->second;
    }
    return nullptr;
}

void SemanticAnalyzer::RegisterUnaryOperator(const TokenType op, const Type* operand, const Type* result)
{
    unary_operators[{op, operand}] = result;
}

const Type* SemanticAnalyzer::LookupUnaryOperator(const TokenType op, const Type* operand) const
{
    if (const auto it = unary_operators.find({op, operand}); it != unary_operators.end())
    {
        return it->second;
    }
    return nullptr;
}

void SemanticAnalyzer::InitializeDefaults()
{
    // Define the global types
    symbol_table.DefineType(PrimitiveType::Int->GetName(), PrimitiveType::Int.get());
    symbol_table.DefineType(PrimitiveType::Float->GetName(), PrimitiveType::Float.get());
    symbol_table.DefineType(PrimitiveType::Bool->GetName(), PrimitiveType::Bool.get());
    symbol_table.DefineType(PrimitiveType::String->GetName(), PrimitiveType::String.get());
    symbol_table.DefineType(PrimitiveType::Void->GetName(), PrimitiveType::Void.get());

    const Type* int_t = PrimitiveType::Int.get();
    const Type* float_t = PrimitiveType::Float.get();
    const Type* bool_t = PrimitiveType::Bool.get();
    const Type* string_t = PrimitiveType::String.get();

    // Arithmetic
    for (const auto op : {TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash})
    {
        RegisterBinaryOperator(op, int_t, int_t, int_t);
        RegisterBinaryOperator(op, float_t, float_t, float_t);
    }

    // String concatenation
    RegisterBinaryOperator(TokenType::Plus, string_t, string_t, string_t);

    // Logical (bool)
    for (const auto op : {TokenType::OrOr, TokenType::AndAnd})
    {
        RegisterBinaryOperator(op, bool_t, bool_t, bool_t);
    }

    // Comparison
    for (const auto op : {TokenType::Less, TokenType::LessEqual, TokenType::Greater, TokenType::GreaterEqual})
    {
        RegisterBinaryOperator(op, int_t, int_t, bool_t);
        RegisterBinaryOperator(op, float_t, float_t, bool_t);
        RegisterBinaryOperator(op, int_t, float_t, bool_t);
        RegisterBinaryOperator(op, float_t, int_t, bool_t);
    }

    // Equality
    for (const auto op : {TokenType::Equal, TokenType::NotEqual})
    {
        RegisterBinaryOperator(op, int_t, int_t, bool_t);
        RegisterBinaryOperator(op, float_t, float_t, bool_t);
        RegisterBinaryOperator(op, bool_t, bool_t, bool_t);
        RegisterBinaryOperator(op, string_t, string_t, bool_t);
    }

    RegisterUnaryOperator(TokenType::Negate, bool_t, bool_t);

    RegisterUnaryOperator(TokenType::Minus, int_t, int_t);
    RegisterUnaryOperator(TokenType::Minus, float_t, float_t);

}

