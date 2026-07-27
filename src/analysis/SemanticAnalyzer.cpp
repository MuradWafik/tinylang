#include "analysis/SemanticAnalyzer.h"

#include <ranges>
#include <unordered_set>

#include "analysis/Type.h"
#include "frontend/Expression.h"
#include "frontend/Statement.h"


std::expected<void, std::string> SemanticAnalyzer::Analyze(const std::vector<std::unique_ptr<ASTNode>>& program)
{
    // Initialize global scope
    symbol_table.PushScope();
    InitializeDefaults();

    for(auto& node : program)
    {
        if(strict_mode)
        {
            if(auto* stmt = dynamic_cast<Statement*>(node.get()))
            {
                if(!dynamic_cast<FunctionDeclaration*>(stmt) && 
                   !dynamic_cast<StructDeclaration*>(stmt) && 
                   !dynamic_cast<VariableDeclaration*>(stmt) &&
                   !dynamic_cast<NativeFunctionDeclaration*>(stmt) &&
                   !dynamic_cast<NativeModuleStatement*>(stmt))
                {
                    return std::unexpected(std::format("Error at line {}: Top-level execution statements are forbidden in strict mode. Use 'fn main()' instead.", stmt->source_location.line_number));
                }
            }
        }

        if(auto result = AnalyzeNode(node.get()); !result)
        {
            return std::unexpected(result.error());
        }
    }
    if(strict_mode)
    {
        if(const auto main_type = symbol_table.LookupVariable("main"))
        {
            if(auto* func_type = dynamic_cast<const FunctionType*>(main_type.value().type))
            {
                if(func_type->GetReturnType() != PrimitiveType::Void.get())
                {
                    return std::unexpected("Error: 'main' must return 'void'.");
                }
            }
            else
            {
                return std::unexpected("Error: 'main' must be a function, not a variable.");
            }
        }
        else
        {
            return std::unexpected("Error: Program must contain a 'fn main()' entrypoint.");
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
    if(const auto* struct_decl = dynamic_cast<StructDeclaration*>(stmt)) return AnalyzeStructDeclaration(struct_decl);

    return std::unexpected(std::format("Unknown statement type '{}'", stmt->GetTypeString()));
}


std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeExpression(Expression* expr)
{
    if(auto* unary_expr = dynamic_cast<UnaryExpression*>(expr)) return AnalyzeUnaryExpression(unary_expr);
    if(auto* binary_expr = dynamic_cast<BinaryExpression*>(expr)) return AnalyzeBinaryExpression(binary_expr);
    if(auto* id_expr = dynamic_cast<IdentifierExpression*>(expr)) return AnalyzeIdentifierExpression(id_expr);
    if(auto* assign_expr = dynamic_cast<AssignmentExpression*>(expr)) return AnalyzeAssignmentExpression(assign_expr);
    if(auto* call_expr = dynamic_cast<CallExpression*>(expr)) return AnalyzeCallExpression(call_expr);
    if(auto* array_node = dynamic_cast<ArrayLiteral*>(expr)) return AnalyzeArrayLiteral(array_node);
    if(auto* index_access = dynamic_cast<IndexAccess*>(expr)) return AnalyzeIndexAccess(index_access);
    if(auto* property_access = dynamic_cast<PropertyAccess*>(expr)) return AnalyzePropertyAccess(property_access);

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

    return std::unexpected("Unknown expression type ");
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    const Type* type = nullptr;
    // with type inference the type defaults to null
    if (variable_declaration->type != "null")
    {
        type = ResolveType(variable_declaration->type);
        if(!type)
        {
            return std::unexpected(std::format("Unknown type name {}", variable_declaration->type));
        }
    }

    if(symbol_table.IsDeclaredInCurrentScope(variable_declaration->name))
    {
        return std::unexpected(std::format("Redefinition of variable {}", variable_declaration->name));
    }

    if(variable_declaration->initializer)
    {
        auto expression_type = AnalyzeExpression(variable_declaration->initializer.get());
        if(!expression_type)
        {
            return std::unexpected(expression_type.error());
        }

        if(type == nullptr)
        {
            // Type inference so the variable takes the type of its initializer
            type = expression_type.value();
        }
        else if(!(expression_type.value())->IsAssignableTo(type))
        {
            return std::unexpected(
                std::format("Unable to assign {} to type {}", expression_type.value()->GetName(), type->GetName())
            );
        }
    }
    else
    {
        if(type == nullptr)
        {
            return std::unexpected(std::format("Cannot infer type of uninitialized variable {}", variable_declaration->name));
        }
    }

    const_cast<VariableDeclaration*>(variable_declaration)->type_info = type;
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

const Type* SemanticAnalyzer::ResolveType(const std::string_view type_name)
{
    if(auto* found = symbol_table.LookupType(type_name))
    {
        return found;
    }

    // define arrays based off the type they are a collection of, to then also check if that type exists
    if(type_name.ends_with("[]"))
    {
        const auto base_type_name = type_name.substr(0, type_name.length() - 2);
        if(const auto* base_type = ResolveType(base_type_name))
        {
            allocated_types.push_back(std::make_unique<ArrayType>(base_type));
            const auto* array_type = allocated_types.back().get();
            symbol_table.DefineType(std::string(type_name), array_type);
            return array_type;
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

    const auto* return_type = ResolveType(function_declaration->return_type);
    if(!return_type)
    {
        return Return(std::format("Unknown return type '{}'", function_declaration->return_type));
    }
    const_cast<FunctionDeclaration*>(function_declaration)->return_type_info = return_type;

    symbol_table.PushScope();


    std::vector<const Type*> parameter_types;
    if(function_declaration->receiver)
    {
        auto& [name, type_name, type_info] = const_cast<Parameter&>(function_declaration->receiver.value());
        auto* resolved_type = ResolveType(type_name);
        if(!resolved_type)
        {
            return Return(std::format("Unknown type '{}' for receiver", type_name));
        }
        type_info = resolved_type;
        parameter_types.push_back(type_info); // receiver has to be the first element
    }

    // then either way the regular parameters get added to the vector
    for(auto& [name, type_name, type_info]: const_cast<FunctionDeclaration*>(function_declaration)->parameters)
    {
        auto* type = ResolveType(type_name);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}'", type_name, name));
        }
        type_info = type;
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);
    symbol_table.DefineType(func_type->GetName(), func_type.get());
    symbol_table.DefineVariable({function_declaration->name, func_type.get()});
    allocated_types.push_back(std::move(func_type));

    symbol_table.PushScope();
    // once in the body those variables are in the scope
    // also make sure `self` is the first variable for the method
    if(function_declaration->receiver)
    {
        auto& [name, type_name, type_info] = function_declaration->receiver.value();
        symbol_table.DefineVariable({name, type_info});
    }

    for(const auto& param: function_declaration->parameters)
    {
        symbol_table.DefineVariable({param.name, param.type_info});
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

    if(return_statement->IsVoidReturn())
    {
        if(current_function_return_type != PrimitiveType::Void.get())
        {
            return Return("Return statement missing value");
        }
        return {};
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

    const auto* return_type = ResolveType(native_function_declaration->return_type);
    if(!return_type)
    {
        return Return(std::format("Unknown return type '{}'", native_function_declaration->return_type));
    }
    const_cast<NativeFunctionDeclaration*>(native_function_declaration)->return_type_info = return_type;

    std::vector<const Type*> parameter_types;
    for(auto& param: const_cast<NativeFunctionDeclaration*>(native_function_declaration)->parameters)
    {
        auto* type = ResolveType(param.type_name);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}'", param.type_name, param.name));
        }
        param.type_info = type;
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);

    symbol_table.DefineType(func_type->GetName(), func_type.get());
    symbol_table.DefineVariable({native_function_declaration->name, func_type.get()});

    allocated_types.push_back(std::move(func_type));
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeStructDeclaration(const StructDeclaration* struct_declaration)
{
    if(symbol_table.LookupType(struct_declaration->name))
    {
        return std::unexpected(std::format("Redefinition of struct '{}'", struct_declaration->name));
    }

    // to throw an error if 2 variables have the same name in definition
    std::unordered_set<std::string_view> seen_names;
    seen_names.reserve(struct_declaration->fields.size());
    std::vector<std::pair<std::string, const Type*>> result;
    result.reserve(struct_declaration->fields.size());
    for(auto& [name, type_name] : struct_declaration->fields)
    {
        if (!seen_names.insert(name).second)
        {
            return std::unexpected(std::format(
                "Duplicate field name '{}' in definition of struct '{}'",
                name, struct_declaration->name)
            );
        }

        auto* type = ResolveType(type_name);
        if(!type)
        {
            return std::unexpected(std::format(
                "Unknown typename '{}' for variable '{}' in definition of struct '{}'",
                type_name, name, struct_declaration->name)
            );
        }
        result.emplace_back(name, type);
    }

    allocated_types.push_back(std::make_unique<StructType>(struct_declaration->name, std::move(result)));
    symbol_table.DefineType(struct_declaration->name, allocated_types.back().get());

    return {};
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeBinaryExpression(BinaryExpression* binary_expression)
{
    const auto left = AnalyzeExpression(binary_expression->left.get());
    if(!left)
    {
        return Return(std::format("Error in left side of binary expression: {}", left.error()));
    }

    const auto right = AnalyzeExpression(binary_expression->right.get());
    if(!right)
    {
        return Return(std::format("Error in right side of binary expression: {}", right.error()));
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
    const auto lhs = AnalyzeExpression(assignment_expression->target.get());
    if(!lhs)
    {
        return Return(lhs.error());
    }

    auto rhs = AnalyzeExpression(assignment_expression->value.get());
    if(!rhs)
    {
        return Return(std::format("Unable to parse right hand side of assignment, {}", rhs.error()));
    }

    if(!rhs.value()->IsAssignableTo(lhs.value()))
    {
        return Return(std::format("Type mismatch: Cannot assign '{}' to '{}'", rhs.value()->GetName(), lhs.value()->GetName()));
    }
    assignment_expression->type_info = rhs.value();
    return rhs.value();
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeCallExpression(CallExpression* call_expression)
{
    const auto callable = AnalyzeExpression(call_expression->callee.get());
    if(!callable)
    {
        // calling methods as it resolves to a call expression with a property access
        if(const auto method_call = dynamic_cast<PropertyAccess*>(call_expression->callee.get()))
        {
            auto obj = AnalyzeExpression(method_call->object_expr.get());
            if(!obj) return std::unexpected(obj.error());
            const std::string method_name = MangleMethodName(method_call, obj.value());
            const auto func_type = symbol_table.LookupVariable(method_name);
            if(!func_type)
            {
                return Return(
                    std::format("Struct '{}' does not have method '{}'", obj.value()->GetName(), method_call->property_name)
                );
            }

            // make the `self` the first arguement to the function call
            call_expression->arguments.insert(
                call_expression->arguments.begin(),
                std::move(method_call->object_expr)
            );

            // Turn the callee into a normal Identifier matching the mangled name
            call_expression->callee = std::make_unique<IdentifierExpression>(
                method_name,
                method_call->source_location
            );
            call_expression->callee->type_info = func_type.value().type;
            return AnalyzeCallExpression(call_expression); // can just run like a regular method call
        }

        return Return(std::format("Unknown call expression '{}'", call_expression->GetTypeString()));
    }

    if(const auto* func_type = dynamic_cast<const FunctionType*>(callable.value()))
    {
        std::string function_name = "<anonymous>";
        if(const auto* real_name = dynamic_cast<IdentifierExpression*>(call_expression->callee.get()))
        {
            function_name = real_name->name;
        }

        if(func_type->GetParameters().size() != call_expression->arguments.size())
        {
            return Return(std::format("Argument count mismatch for {}: expected {}, got {}",
                function_name, func_type->GetParameters().size(), call_expression->arguments.size()));
        }

        for(auto [param, arg] : std::views::zip(func_type->GetParameters(), call_expression->arguments))
        {
            auto arg_type = AnalyzeExpression(arg.get());
            if(!arg_type)
            {
                return Return(std::format(
                    "Error parsing function '{}' parameter expression, {}",
                    function_name, arg_type.error()));
            }
            if(!arg_type.value()->IsAssignableTo(param))
            {
                return Return(
                    std::format("Error calling function '{}': Type '{}' is not assignable to '{}'",
                    function_name, arg_type.value()->GetName(), param->GetName()));
            }
        }

        call_expression->type_info = func_type->GetReturnType();
        return func_type->GetReturnType();
    }
    else if(const auto* struct_type = dynamic_cast<const StructType*>(callable.value()))
    {
        if(call_expression->arguments.size() != struct_type->GetNumFields())
        {
            return Return("Too much arguments passed for struct initialization");
        }
        for(
            const auto& [field, expression] :
            std::ranges::views::zip(struct_type->GetFields(), call_expression->arguments))
        {
            auto expression_type = AnalyzeExpression(expression.get());
            if(!expression_type)
            {
                return Return(expression_type.error());
            }
            if(expression_type.value() != field.second)
            {
                return Return(
                    std::format("Type mismatch in struct initialization, expected '{}', got '{}' ",
                        field.second->GetName(), expression_type.value()->GetName())
                );
            }

            call_expression->type_info = struct_type;
        }
    }

    return Return("Attempted to call a value that is not a function or struct");
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
    
    RegisterBinaryOperator(TokenType::Modulo, int_t, int_t, int_t);

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

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeIndexAccess(IndexAccess* index_access)
{
    auto array_type = AnalyzeExpression(index_access->array_expr.get());
    if(!array_type)
    {
        return std::unexpected(array_type.error());
    }

    auto* array_t = dynamic_cast<const ArrayType*>(array_type.value());
    if(!array_t)
    {
        return Return("Cannot index into a non-array type");
    }

    auto index_type = AnalyzeExpression(index_access->index_expr.get());
    if(!index_type) return std::unexpected(index_type.error());
    if(index_type.value() != PrimitiveType::Int.get())
    {
        return Return("Array index must be an integer");
    }

    // The type of the IndexAccess is the element type
    index_access->type_info = array_t->GetElementType();
    return index_access->type_info;
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeArrayLiteral(ArrayLiteral* array_node)
{
    if (array_node->elements.empty())
    {
        return Return("Cannot infer type of empty array literal");
    }

    auto first_type = AnalyzeExpression(array_node->elements[0].get());
    if (!first_type)
    {
        return std::unexpected(first_type.error());
    }

    for(size_t i = 1; i < array_node->elements.size(); ++i)
    {
        auto elem_type = AnalyzeExpression(array_node->elements[i].get());
        if (!elem_type) return std::unexpected(elem_type.error());

        if(elem_type.value() != first_type.value())
        {
            return Return("Array literal elements must all be of the same type");
        }
    }

    allocated_types.push_back(std::make_unique<ArrayType>(first_type.value()));
    array_node->type_info = allocated_types.back().get();
    return array_node->type_info;
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzePropertyAccess(PropertyAccess* property_access)
{
    auto lhs = AnalyzeExpression(property_access->object_expr.get());
    if(!lhs)
    {
        return std::unexpected(lhs.error());
    }
    if(dynamic_cast<const ArrayType*>(lhs.value()) || lhs.value() == PrimitiveType::String.get())
    {
        if(property_access->property_name == "length")
        {
            property_access->type_info = PrimitiveType::Int.get();
            return property_access->type_info;
        }
        return Return(std::format("Type '{}' only has a 'length' property", lhs.value()->GetName()));
    }

    const auto struct_obj = dynamic_cast<const StructType*>(lhs.value());
    if(!struct_obj)
    {
        return Return(std::format("Trying to do property access on non struct type '{}'", lhs.value()->GetName()));
    }

    const auto field_type = struct_obj->GetFieldType(property_access->property_name);
    if(!field_type)
    {
        return Return(std::format(
            "Struct '{}' does not contain field '{}'",
            struct_obj->GetName(), property_access->property_name)
        );
    }

    property_access->type_info = field_type;
    return field_type;
}
