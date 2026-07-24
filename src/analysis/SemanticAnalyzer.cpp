#include "analysis/SemanticAnalyzer.h"

#include <ranges>

#include "analysis/Type.h"
#include "frontend/Expression.h"
#include "frontend/Statement.h"


std::expected<void, std::string> SemanticAnalyzer::Analyze(const std::vector<std::unique_ptr<ASTNode>>& program)
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

std::expected<void, std::string> SemanticAnalyzer::Analyze(ASTNode* node)
{
    // Initialize global scope
    symbol_table.PushScope();
    InitializeDefaults();

    auto result = AnalyzeNode(node);

    symbol_table.PopScope();
    return result;
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
        // expressions return an expected type for their recursive chain, can just be ignored here
        if (auto res = AnalyzeExpression(expr); !res) return std::unexpected(res.error());
        return {};
    }
    return std::unexpected("Unknown node type");

}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeStatement(Statement* stmt)
{
    if(const auto* var_decl = dynamic_cast<VariableDeclaration*>(stmt)) return AnalyzeVariableDeclaration(var_decl);
    if(const auto* if_stmt = dynamic_cast<IfStatement*>(stmt)) return AnalyzeIfStatement(if_stmt);
    if(const auto* while_stmt = dynamic_cast<WhileStatement*>(stmt)) return AnalyzeWhileStatement(while_stmt);
    if(const auto* fn_declaration = dynamic_cast<FunctionDeclaration*>(stmt)) return AnalyzeFunctionDeclaration(fn_declaration);
    if(const auto* break_stmt = dynamic_cast<BreakStatement*>(stmt)) return AnalyzeBreakStatement(break_stmt);
    if(const auto* continue_stmt = dynamic_cast<ContinueStatement*>(stmt)) return AnalyzeContinueStatement(continue_stmt);
    if(const auto* return_stmt = dynamic_cast<ReturnStatement*>(stmt)) return AnalyzeReturnStatement(return_stmt);
    if(const auto* body_stmt = dynamic_cast<BodyStatement*>(stmt)) return AnalyzeBodyStatement(body_stmt);
    if(const auto* expr_stmt = dynamic_cast<ExpressionStatement*>(stmt)) return AnalyzeExpressionStatement(expr_stmt);
    if(const auto* native_mod_stmt = dynamic_cast<NativeModuleStatement*>(stmt)) return AnalyzeNativeModuleStatement(native_mod_stmt);
    if(const auto* native_fn_decl = dynamic_cast<NativeFunctionDeclaration*>(stmt)) return AnalyzeNativeFunctionDeclaration(native_fn_decl);

    return std::unexpected(std::format("Unknown statement type '{}'", stmt->GetTypeString()));
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeExpression(Expression* expr)
{
    if(auto* unary_expr = dynamic_cast<UnaryExpression*>(expr)) return AnalyzeUnaryExpression(unary_expr);
    if(auto* binary_expr = dynamic_cast<BinaryExpression*>(expr)) return AnalyzeBinaryExpression(binary_expr);
    if(auto* id_expr = dynamic_cast<IdentifierExpression*>(expr)) return AnalyzeIdentifierExpression(id_expr);
    if(auto* assign_expr = dynamic_cast<AssignmentExpression*>(expr)) return AnalyzeAssignmentExpression(assign_expr);
    if(auto* call_expr = dynamic_cast<CallExpression*>(expr)) return AnalyzeCallExpression(call_expr);
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

    if(symbol_table.IsDeclaredInCurrentScope(variable_declaration->name))
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
    symbol_table.DefineVariable({variable_declaration->name, type});
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

bool SymbolTable::IsDeclaredInCurrentScope(const std::string_view name) const
{
    return scopes.back().variables.contains(name);
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

std::expected<void, std::string> SemanticAnalyzer::AnalyzeFunctionDeclaration(
    const FunctionDeclaration* function_declaration)
{
    // cache what the return type was before in case its nested
    const auto* outer_return = current_function_return_type;

    if(symbol_table.IsDeclaredInCurrentScope(function_declaration->name))
    {
        return Return(std::format("Redefinition of variable/function '{}'", function_declaration->name));
    }

    const auto* return_type = symbol_table.LookupType(function_declaration->return_type);
    if(!return_type)
    {
        return Return(std::format("Unknown return type '{}'", function_declaration->return_type));
    }

    std::vector<const Type*> parameter_types;
    for(const auto& [name, type_name]: function_declaration->parameters)
    {
        auto* type = symbol_table.LookupType(type_name);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}'", type_name, name));
        }
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);

    symbol_table.DefineType(func_type->GetName(), func_type.get());
    symbol_table.DefineVariable({function_declaration->name, func_type.get()});

    // once in the body those variables are in the scope
    symbol_table.PushScope();
    for(const auto& [param_name, param_type_name]: function_declaration->parameters)
    {
        // garunteed to exist based on earlier check
        const auto* param_type = symbol_table.LookupType(param_type_name);
        symbol_table.DefineVariable({param_name, param_type});
    }

    current_function_return_type = return_type;

    if(auto body = AnalyzeStatement(function_declaration->body.get()); !body)
    {
        return std::unexpected(body.error());
    }
    symbol_table.PopScope();

    current_function_return_type = outer_return; // return to its previous value (even if it was null)
    allocated_types.push_back(std::move(func_type));

    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeBreakStatement(const BreakStatement* break_statement) const
{
    if(loop_depth <= 0)
    {
        return Return("Breaking outside of a loop");
    }
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeContinueStatement(const ContinueStatement* continue_statement) const
{
    if(loop_depth <= 0)
    {
        return Return("Continuing outside of a loop");
    }
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeReturnStatement(const ReturnStatement* return_statement)
{
    if(!current_function_return_type)
    {
        return Return("Return statement outside of function body");
    }

    auto return_value = AnalyzeExpression(return_statement->value.get());
    if(!return_value)
    {
        return Return(std::format("Unable to parse return expression, {}", return_value.error()));
    }

    if(!return_value.value()->IsAssignableTo(current_function_return_type))
    {
        return Return(
            std::format("Unable to assign return type '{}' to function return type '{}'",
                return_value.value()->GetName(), current_function_return_type->GetName())
        );
    }

    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeBodyStatement(const BodyStatement* body_statement)
{
    symbol_table.PushScope();
    for(const auto& statement: body_statement->statements)
    {
        if (auto result = AnalyzeNode(statement.get()); !result)
        {
            symbol_table.PopScope();
            return std::unexpected(result.error());
        }
    }
    symbol_table.PopScope();
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeExpressionStatement(
        const ExpressionStatement* expression_statement)
{
    return Analyze(expression_statement->expression.get());
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeNativeModuleStatement(const NativeModuleStatement* native_module_statement)
{
    if(!current_native_module.empty())
    {
        return std::unexpected<std::string>("Only one native module definition is allowed");
    }

    const auto mod_path = project_config->ResolvePluginPath(native_module_statement->name);
    if(mod_path.empty())
    {
        return std::unexpected(std::format("Unable to locate module '{}'", native_module_statement->name));
    }

    current_native_module = mod_path.string();;
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeNativeFunctionDeclaration(const NativeFunctionDeclaration* native_function_declaration)
{
    if(current_native_module.empty())
    {
        return std::unexpected(
            std::format("No native module for native function declaration '{}'", native_function_declaration->name)
        );
    }

    if (symbol_table.GetScopeDepth() > 0)
    {
        return std::unexpected("Native functions can only be declared at the global scope");
    }

    if(symbol_table.IsDeclaredInCurrentScope(native_function_declaration->name))
    {
        return Return(std::format("Redefinition of variable/function '{}'", native_function_declaration->name));
    }

    const auto* return_type = symbol_table.LookupType(native_function_declaration->return_type);
    if(!return_type)
    {
        return Return(std::format("Unknown return type '{}'", native_function_declaration->return_type));
    }

    std::vector<const Type*> parameter_types;
    for(const auto& [name, type_name]: native_function_declaration->parameters)
    {
        auto* type = symbol_table.LookupType(type_name);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}'", type_name, name));
        }
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);

    symbol_table.DefineType(func_type->GetName(), func_type.get());
    symbol_table.DefineVariable({native_function_declaration->name, func_type.get()});

    allocated_types.push_back(std::move(func_type));
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

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeIdentifierExpression(
    IdentifierExpression* identifier_expression)
{
    const auto& identifier = symbol_table.LookupVariable(identifier_expression->name);
    if(!identifier)
    {
        return std::unexpected(std::format("Unknown symbol: {}", identifier_expression->name));
    }
    identifier_expression->type_info = identifier->type;
    return identifier->type;
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeAssignmentExpression(
    AssignmentExpression* assignment_expression)
{
    auto lhs = symbol_table.LookupVariable(assignment_expression->name);
    if(!lhs)
    {
        return Return(std::format("Use of undeclared identifier '{}'", assignment_expression->name));
    }

    auto rhs = AnalyzeExpression(assignment_expression->value.get());
    if(!rhs)
    {
        return Return(std::format("Unable to parse right hand side of assignment, {}", rhs.error()));
    }

    if(!rhs.value()->IsAssignableTo(lhs->type))
    {
        return Return(std::format("Unable to assign type '{}' to '{}'", rhs.value()->GetName(), lhs->type->GetName()));
    }

    return PrimitiveType::Void.get();
    // According to AI, best to NOT then return the value and instead return void, disallowing chaining `x = y = 10;`
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeCallExpression(CallExpression* call_expression)
{
    const auto func = symbol_table.LookupVariable(call_expression->function_name);
    if(!func)
    {
        return Return(std::format("Unknown function '{}'", call_expression->function_name));
    }


    const auto* func_type = dynamic_cast<const FunctionType*>(func->type);
    if(!func_type)
    {
        return Return(std::format("Calling a non-function '{}'", func->name));
    }

    if (func_type->GetParameters().size() != call_expression->arguments.size()) {
        return Return(std::format("Argument count mismatch for '{}': expected {}, got {}",
            call_expression->function_name, func_type->GetParameters().size(), call_expression->arguments.size()));
    }


    for (auto [param, arg] : std::views::zip(func_type->GetParameters(), call_expression->arguments))
    {
        auto arg_type = AnalyzeExpression(arg.get());
        if(!arg_type)
        {
            return Return(std::format("Error parsing function parameter expression, {}", arg_type.error()));
        }
        if(!arg_type.value()->IsAssignableTo(param))
        {
            return Return(
                std::format("Error calling function '{}', Type '{}' is not assignable to '{}'",
                    call_expression->function_name, arg_type.value()->GetName(), param->GetName()));
        }
    }

    call_expression->type_info = func_type->GetReturnType();
    return func_type->GetReturnType();
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


    // TODO: Reimplement native functions
    // NativeFunction::RegisterTypes(symbol_table);
}

