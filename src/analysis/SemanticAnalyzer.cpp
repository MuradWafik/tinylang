#include "analysis/SemanticAnalyzer.h"

#include <ranges>
#include <unordered_set>

#include "analysis/Type.h"
#include "frontend/Expression.h"
#include "frontend/Statement.h"
#include "utils/Constants.h"


std::string GetNameFromExportable(const ExportableStatement* stmt)
{
    if(const auto fn_decl = dynamic_cast<const FunctionDeclaration*>(stmt)) return fn_decl->method_signature.name.lexeme;
    if(const auto struct_decl = dynamic_cast<const StructDeclaration*>(stmt)) return struct_decl->name.lexeme;
    if(const auto var = dynamic_cast<const VariableDeclaration*>(stmt)) return var->name.lexeme;
    if(const auto enum_decl = dynamic_cast<const EnumDeclaration*>(stmt)) return enum_decl->name.lexeme;
    if(const auto intf_decl = dynamic_cast<const InterfaceDeclaration*>(stmt)) return intf_decl->name.lexeme;
    return "Unknown exportable statement " + stmt->GetTypeString();
}

std::string SemanticAnalyzer::MangleName(const std::string& name) const
{
    if(current_namespace == "main" || current_namespace.empty()) return name;
    return Mangling::ModuleSymbol(current_namespace, name);
}

std::expected<void, std::string> SemanticAnalyzer::RunAnalysis()
{
    for(const auto& [name, namespace_obj] : module_registry->GetNamespaces())
    {
        current_namespace = name;
        for(const auto& ast_node : namespace_obj->asts)
        {
            if(auto result = AnalyzeNode(ast_node.get()); !result)
            {
                return std::unexpected(result.error());
            }
        }
    }
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeAll()
{
    symbol_table.PushScope();
    InitializeDefaults();

    analysis_pass = AnalysisPass::Registration;
    
    for(const auto& [name, namespace_obj] : module_registry->GetNamespaces())
    {
        // Register module type so it can be accessed
        if (name != "main") {
            auto mod_type = std::make_unique<ModuleType>(name);
            symbol_table.DefineType(name, mod_type.get());
            symbol_table.DefineVariable({name, mod_type.get()});
            allocated_types.push_back(std::move(mod_type));
        }
        
        current_namespace = name;
        current_native_module = ""; // Reset for each namespace so it doesn't bleed across different modules
        
        for(const auto& ast_node : namespace_obj->asts)
        {
            if(auto result = AnalyzeNode(ast_node.get()); !result) return std::unexpected(result.error());

            if(const auto exportable = dynamic_cast<ExportableStatement*>(ast_node.get());
                exportable && exportable->is_exported)
            {
                std::string symbol_name = GetNameFromExportable(exportable);
                module_registry->RegisterExport(name, symbol_name, exportable);
            }
        }
    }
    analysis_pass = AnalysisPass::Validation;
    if(auto second_pass = RunAnalysis(); !second_pass) return second_pass;

    const auto main = symbol_table.LookupVariable("main");
    if(!main)
    {
        return std::unexpected("no main function detected");
    }

    const auto main_fn = dynamic_cast<const FunctionType*>(main->type);
    if(!main_fn)
    {
        return std::unexpected("main() must be a function");
    }

    if(main_fn->GetReturnType() != PrimitiveType::Void.get())
    {
        return std::unexpected("main function must have a return type of void");
    }

    if(main_fn->GetParameters().size() != 0)
    {
        return std::unexpected("main function must not have any parameters");
    }

    if(auto interfaces_implemented = EnsureInterfacesImplemented(); !interfaces_implemented) return interfaces_implemented;
    symbol_table.PopScope();

    return {};
}


std::expected<void, std::string> SemanticAnalyzer::AnalyzeNode(ASTNode* node)
{
    // let statements and expressions handle their own dispatching
    if(auto* stmt = dynamic_cast<Statement*>(node))
    {
        return AnalyzeStatement(stmt);
    }
    if(auto* expr = dynamic_cast<Expression*>(node))
    {
        // expressions return an expected type for their recursive chain, can just be ignored here
        if(auto res = AnalyzeExpression(expr); !res) return std::unexpected(res.error());
        return {};
    }
    return std::unexpected("Unknown node type");

}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeStatement(Statement* stmt)
{
    if(const auto* var_decl = dynamic_cast<VariableDeclaration*>(stmt)) return AnalyzeVariableDeclaration(var_decl);
    if(const auto* if_stmt = dynamic_cast<IfStatement*>(stmt)) return AnalyzeIfStatement(if_stmt);
    if(const auto* while_stmt = dynamic_cast<WhileStatement*>(stmt)) return AnalyzeWhileStatement(while_stmt);
    if(auto* fn_declaration = dynamic_cast<FunctionDeclaration*>(stmt)) return AnalyzeFunctionDeclaration(fn_declaration);
    if(const auto* break_stmt = dynamic_cast<BreakStatement*>(stmt)) return AnalyzeBreakStatement(break_stmt);
    if(const auto* continue_stmt = dynamic_cast<ContinueStatement*>(stmt)) return AnalyzeContinueStatement(continue_stmt);
    if(const auto* return_stmt = dynamic_cast<ReturnStatement*>(stmt)) return AnalyzeReturnStatement(return_stmt);
    if(const auto* body_stmt = dynamic_cast<BodyStatement*>(stmt)) return AnalyzeBodyStatement(body_stmt);
    if(const auto* expr_stmt = dynamic_cast<ExpressionStatement*>(stmt)) return AnalyzeExpressionStatement(expr_stmt);
    
    // These statements don't have semantic meaning beyond their module-level grouping handled by the parser/registry
    if(dynamic_cast<ModuleDeclaration*>(stmt) || dynamic_cast<ImportStatement*>(stmt)) return {};

    if(const auto* native_mod_stmt = dynamic_cast<NativeImportStatement*>(stmt)) return AnalyzeNativeModuleStatement(native_mod_stmt);
    if(auto* native_fn_decl = dynamic_cast<NativeFunctionDeclaration*>(stmt)) return AnalyzeNativeFunctionDeclaration(native_fn_decl);
    if(auto* struct_decl = dynamic_cast<StructDeclaration*>(stmt)) return AnalyzeStructDeclaration(struct_decl);
    if(const auto* enum_decl = dynamic_cast<EnumDeclaration*>(stmt)) return AnalyzeEnumDeclaration(enum_decl);
    if(auto* interface_decl = dynamic_cast<InterfaceDeclaration*>(stmt)) return AnalyzeInterfaceDeclaration(interface_decl);
    if(const auto* for_loop = dynamic_cast<ForLoop*>(stmt)) return AnalyzeForLoop(for_loop);
    if(const auto* extend_stmt = dynamic_cast<ExtendStatement*>(stmt)) return AnalyzeExtendStatement(extend_stmt);

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
    if(auto* switch_expr = dynamic_cast<SwitchExpression*>(expr)) return AnalyzeSwitchExpression(switch_expr);
    if(auto* cast_expr = dynamic_cast<CastExpression*>(expr)) return AnalyzeCastExpression(cast_expr);
    if(auto* is_expr = dynamic_cast<IsExpression*>(expr)) return AnalyzeIsExpression(is_expr);

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
    if(auto* char_node = dynamic_cast<CharLiteral*>(expr))
    {
        char_node->type_info = PrimitiveType::Char.get();
        return PrimitiveType::Char.get();
    }

    return std::unexpected(std::format("Unknown expression type '{}'", expr->GetTypeString()));
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    if (symbol_table.GetScopeDepth() == 0) {
        const_cast<VariableDeclaration*>(variable_declaration)->name.lexeme = MangleName(variable_declaration->name.lexeme);
    }
    
    const Type* type = nullptr;
    // with type inference the type defaults to null
    if(variable_declaration->type.has_value())
    {
        type = ResolveType(variable_declaration->type->lexeme);
        if(!type)
        {
            return std::unexpected(std::format("Unknown type name {} at {}", variable_declaration->type->lexeme, variable_declaration->type->source_location));
        }
        if(dynamic_cast<const InterfaceType*>(type))
        {
            return std::unexpected(std::format("Cannot use interface '{}' as a concrete type for variable '{}' at {}", variable_declaration->type->lexeme, variable_declaration->name.lexeme, variable_declaration->type->source_location));
        }
        if(type == PrimitiveType::Void.get())
        {
            return std::unexpected(std::format("Cannot use 'void' as a type for variable '{}' at {}", variable_declaration->name.lexeme, variable_declaration->type->source_location));
        }
    }

    if(symbol_table.IsDeclaredInCurrentScope(variable_declaration->name.lexeme))
    {
        return std::unexpected(std::format("Redefinition of variable '{}' at {}", variable_declaration->name.lexeme, variable_declaration->name.source_location));
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
            return std::unexpected(std::format("Cannot infer type of uninitialized variable '{}' at {}", variable_declaration->name.lexeme, variable_declaration->name.source_location));
        }
    }

    const_cast<VariableDeclaration*>(variable_declaration)->type_info = type;
    symbol_table.DefineVariable({variable_declaration->name.lexeme, type});
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

        if(!name.contains("::") && !name.contains("$"))
        {
            std::optional<Symbol> candidate = std::nullopt;
            size_t match_count = 0;
            for(const auto& [v_name, v_symbol] : variables)
            {
                if(v_name.ends_with("::" + std::string(name)))
                {
                    candidate = v_symbol;
                    match_count++;
                }
            }
            if(match_count == 1)
            {
                return candidate;
            }
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
    for(auto & [variables, types] : std::ranges::reverse_view(scopes))
    {
        if(auto found = types.find(name); found != types.end())
        {
            return found->second;
        }

        if(!name.contains("::"))
        {
            const Type* candidate = nullptr;
            size_t match_count = 0;
            for(const auto& [t_name, t_type] : types)
            {
                if(t_name.ends_with("::" + std::string(name)))
                {
                    candidate = t_type;
                    match_count++;
                }
            }
            if(match_count == 1)
            {
                return candidate;
            }
        }
    }
    return nullptr;
}

const Type* SemanticAnalyzer::ResolveType(const std::string_view type_name)
{
    if(!current_namespace.empty() && current_namespace != "main" && !type_name.contains("::"))
    {
        const std::string mangled = Mangling::ModuleSymbol(current_namespace, type_name);
        if(auto* found = symbol_table.LookupType(mangled))
        {
            return found;
        }
    }

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

    if(if_statement->else_branch)
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
        FunctionDeclaration* function_declaration)
{
    if (analysis_pass == AnalysisPass::Validation)
    {
        const auto* outer_return = current_function_return_type;
        current_function_return_type = function_declaration->method_signature.return_type_info;

        symbol_table.PushScope();
        
        if(function_declaration->receiver)
        {
            auto& [recv_name, type_name, type_info] = function_declaration->receiver.value();
            symbol_table.DefineVariable({recv_name.lexeme, type_info});
        }
        for(const auto& param: function_declaration->method_signature.parameters)
        {
            symbol_table.DefineVariable({param.name.lexeme, param.type_info});
        }

        if(auto body = AnalyzeStatement(function_declaration->body.get()); !body)
        {
            return std::unexpected(body.error());
        }

        symbol_table.PopScope();
        current_function_return_type = outer_return;
        return {};
    }

    // kept as a reference when registering it at the end, as the actual name gets mangled if its a method
    const std::string unmangled_name = function_declaration->method_signature.name.lexeme;

    if (analysis_pass == AnalysisPass::Registration)
    {
        // Mangle module-level functions
        if (!function_declaration->receiver && symbol_table.GetScopeDepth() == 0) {
            function_declaration->method_signature.name.lexeme = MangleName(function_declaration->method_signature.name.lexeme);
        }
    }

    auto& name = function_declaration->method_signature.name.lexeme;
    
    if(function_declaration->receiver.has_value())
    {
        auto* self = ResolveType(function_declaration->receiver->type_name.lexeme);
        if(!self)
        {
            return Return(std::format(
                "Unknown type '{}' for receiver at {}",
                function_declaration->receiver->type_name.lexeme, function_declaration->receiver->type_name.source_location));
        }
        if(!name.contains("$"))
        {
            name = Mangling::MethodName(self->GetName(), name);
        }
    }

    // cache what the return type was before in case its nested
    const auto* outer_return = current_function_return_type;
    if(symbol_table.IsDeclaredInCurrentScope(name))
    {
        return Return(std::format("Redefinition of variable/function '{}' at {}", name, function_declaration->method_signature.name.source_location));
    }

    const Type* return_type = PrimitiveType::Void.get();
    if(function_declaration->method_signature.return_type.has_value())
    {
        return_type = ResolveType(function_declaration->method_signature.return_type->lexeme);
        if(!return_type)
        {
            return Return(std::format("Unknown return type '{}'", function_declaration->method_signature.return_type->lexeme));
        }
        if(dynamic_cast<const InterfaceType*>(return_type))
        {
            return Return(std::format(
                "Cannot use interface '{}' as a concrete return type at {}",
                function_declaration->method_signature.return_type->lexeme,
                function_declaration->method_signature.return_type->source_location));
        }
    }
    function_declaration->method_signature.return_type_info = return_type;

    std::vector<const Type*> parameter_types;
    const Type* receiver_type = nullptr; // once the reciever is handled, can register the method with the type afterwards
    if(function_declaration->receiver)
    {
        auto& [recv_name, type_name, type_info] = function_declaration->receiver.value();
        auto* self = ResolveType(type_name.lexeme);
        if(!self)
        {
            return Return(std::format("Unknown type '{}' for receiver at {}", type_name.lexeme, type_name.source_location));
        }
        if(dynamic_cast<const InterfaceType*>(self))
        {
            return Return(std::format("Cannot use interface '{}' as a receiver at {}", type_name.lexeme, type_name.source_location));
        }
        if(self == PrimitiveType::Void.get())
        {
            return Return(std::format("Cannot use 'void' as a receiver at {}", type_name.source_location));
        }
        type_info = self;
        parameter_types.push_back(type_info); // receiver has to be the first element

        receiver_type = self;

    }

    // then either way the regular parameters get added to the vector
    for(auto& [param_name, type_name, type_info]: function_declaration->method_signature.parameters)
    {
        auto* type = ResolveType(type_name.lexeme);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}' at {}", type_name.lexeme, param_name.lexeme, type_name.source_location));
        }
        if(dynamic_cast<const InterfaceType*>(type))
        {
            return Return(std::format("Cannot use interface '{}' as a concrete type for parameter '{}' at {}", type_name.lexeme, param_name.lexeme, type_name.source_location));
        }
        if(type == PrimitiveType::Void.get())
        {
            return Return(std::format("Cannot use 'void' as a type for parameter '{}' at {}", param_name.lexeme, type_name.source_location));
        }
        type_info = type;
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);
    auto* func_ptr = func_type.get();
    symbol_table.DefineType(func_ptr->GetName(), func_ptr);
    symbol_table.DefineVariable({name, func_ptr});
    
    if(function_declaration->receiver)
    {
        const_cast<Type*>(receiver_type)->RegisterMethod(
            unmangled_name,
            func_ptr
        );
    }
    
    allocated_types.push_back(std::move(func_type));

    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeBreakStatement(const BreakStatement*) const
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
        if(auto result = AnalyzeNode(statement.get()); !result)
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
    return AnalyzeNode(expression_statement->expression.get());
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeNativeModuleStatement(const NativeImportStatement* native_module_statement)
{
    if(analysis_pass == AnalysisPass::Validation) return {};

    if(!current_native_module.empty())
    {
        return std::unexpected<std::string>("Only one native module definition is allowed");
    }

    const auto mod_path = project_config->ResolvePluginPath(native_module_statement->name.lexeme);

    if(mod_path.empty())
    {
        return std::unexpected(std::format("Unable to locate module '{}' at {}", native_module_statement->name.lexeme, native_module_statement->name.source_location));
    }

    current_native_module = mod_path.string();;
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeNativeFunctionDeclaration(NativeFunctionDeclaration* native_function_declaration)
{
    if(analysis_pass == AnalysisPass::Validation) return {};

    if (symbol_table.GetScopeDepth() == 0) {
        native_function_declaration->method_signature.name.lexeme = MangleName(native_function_declaration->method_signature.name.lexeme);
    }
    auto& name = native_function_declaration->method_signature.name.lexeme;

    if(current_native_module.empty())
    {
        return std::unexpected(
            std::format("No native module for native function declaration '{}'", name)
        );
    }

    if(symbol_table.GetScopeDepth() > 0)
    {
        return std::unexpected("Native functions can only be declared at the global scope");
    }

    if(symbol_table.IsDeclaredInCurrentScope(name))
    {
        return Return(std::format("Redefinition of variable/function '{}'", name));
    }

    const Type* return_type = PrimitiveType::Void.get();
    if(native_function_declaration->method_signature.return_type.has_value())
    {
        return_type = ResolveType(native_function_declaration->method_signature.return_type->lexeme);
        if(!return_type)
        {
            return Return(std::format("Unknown return type '{}' at {}", native_function_declaration->method_signature.return_type->lexeme, native_function_declaration->method_signature.return_type->source_location));
        }
        if(dynamic_cast<const InterfaceType*>(return_type))
        {
            return Return(std::format("Cannot use interface '{}' as a concrete return type at {}", native_function_declaration->method_signature.return_type->lexeme, native_function_declaration->method_signature.return_type->source_location));
        }
    }
    native_function_declaration->method_signature.return_type_info = return_type;

    std::vector<const Type*> parameter_types;
    for(auto& [param_name, type_name, type_info]: native_function_declaration->method_signature.parameters)
    {
        auto* type = ResolveType(type_name.lexeme);
        if(!type)
        {
            return Return(std::format("Unknown type '{}' for function parameter '{}' at {}", type_name.lexeme, param_name.lexeme, type_name.source_location));
        }
        if(dynamic_cast<const InterfaceType*>(type))
        {
            return Return(std::format("Cannot use interface '{}' as a concrete type for parameter '{}' at {}", type_name.lexeme, param_name.lexeme, type_name.source_location));
        }
        if(type == PrimitiveType::Void.get())
        {
            return Return(std::format("Cannot use 'void' as a type for parameter '{}' at {}", param_name.lexeme, type_name.source_location));
        }
        type_info = type;
        parameter_types.push_back(type);
    }

    auto func_type = std::make_unique<FunctionType>(parameter_types, return_type);

    symbol_table.DefineType(func_type->GetName(), func_type.get());
    symbol_table.DefineVariable({name, func_type.get()});

    allocated_types.push_back(std::move(func_type));
    return {};
}



std::expected<void, std::string> SemanticAnalyzer::AnalyzeStructDeclaration(StructDeclaration* struct_declaration)
{
    if(analysis_pass == AnalysisPass::Validation) return {};

    if (symbol_table.GetScopeDepth() == 0) {
        struct_declaration->name.lexeme = MangleName(struct_declaration->name.lexeme);
    }    
        if (symbol_table.LookupType(struct_declaration->name.lexeme))
    {
        return std::unexpected(std::format("Redefinition of struct '{}' at {}", struct_declaration->name.lexeme, struct_declaration->name.source_location));
    }

    // to throw an error if 2 variables have the same name in definition
    std::unordered_set<std::string_view> seen_names;
    seen_names.reserve(struct_declaration->fields.size());

    // Define the type first to allow self-referential fields (since structs are heap pointers)
    auto struct_type = std::make_unique<StructType>(struct_declaration->name.lexeme, std::vector<std::pair<std::string, const Type*>>{});
    auto* struct_ptr = struct_type.get();
    allocated_types.push_back(std::move(struct_type));
    symbol_table.DefineType(struct_declaration->name.lexeme, struct_ptr);

    std::vector<std::pair<std::string, const Type*>> result;
    result.reserve(struct_declaration->fields.size());

    for(auto& [field_name, type_name] : struct_declaration->fields)
    {
        if(!seen_names.insert(field_name.lexeme).second)
        {
            return std::unexpected(std::format(
                "Duplicate field name '{}' in definition of struct '{}'",
                field_name.lexeme, struct_declaration->name.lexeme)
            );
        }

        auto* type = ResolveType(type_name.lexeme);
        if(!type)
        {
            return std::unexpected(
                std::format("Type '{}' for field '{}' in struct '{}' is unknown! at {}",
                type_name.lexeme, field_name.lexeme, struct_declaration->name.lexeme, type_name.source_location)
            );
        }
        if(dynamic_cast<const InterfaceType*>(type))
        {
            return std::unexpected(std::format("Cannot use interface '{}' as a concrete type for field '{}' in struct '{}' at {}", type_name.lexeme, field_name.lexeme, struct_declaration->name.lexeme, type_name.source_location));
        }
        if(type == PrimitiveType::Void.get())
        {
            return std::unexpected(std::format("Cannot use 'void' as a type for field '{}' in struct '{}' at {}", field_name.lexeme, struct_declaration->name.lexeme, type_name.source_location));
        }
        result.emplace_back(field_name.lexeme, type);
    }

    struct_ptr->SetFields(std::move(result));

    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeEnumDeclaration(const EnumDeclaration* enum_declaration)
{
    if(analysis_pass == AnalysisPass::Validation) return {};

    std::unordered_map<std::string, int32_t> variants;
    auto counter = 0; // default starting value for varint if user did not assign one
    for(const auto& [variant_name, value]: enum_declaration->variant_names)
    {
        if(value.has_value())
        {
            counter = value.value();
        }

        variants.insert({variant_name.lexeme, counter});
        counter++;
    }
    allocated_types.push_back(std::make_unique<EnumType>(enum_declaration->name.lexeme, std::move(variants)));
    symbol_table.DefineType(enum_declaration->name.lexeme, allocated_types.back().get());
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
    // Try looking up in the current namespace first if we are inside one
    if(!current_namespace.empty() && current_namespace != "main")
    {
        const std::string mangled = Mangling::ModuleSymbol(current_namespace, identifier_expression->name);
        if(const auto& identifier = symbol_table.LookupVariable(mangled))
        {
            identifier_expression->name = mangled;
            identifier_expression->type_info = identifier->type;
            return identifier->type;
        }
        
        if (const auto type = symbol_table.LookupType(mangled))
        {
            identifier_expression->name = mangled;
            identifier_expression->type_info = type;
            return type;
        }
    }

    if(const auto& identifier = symbol_table.LookupVariable(identifier_expression->name))
    {
        identifier_expression->name = identifier->name;
        identifier_expression->type_info = identifier->type;
        return identifier->type;
    }
    
    if(const auto type = symbol_table.LookupType(identifier_expression->name))
    {
        identifier_expression->name = type->GetName();
        identifier_expression->type_info = type;
        return type;
    }

    return std::unexpected(std::format("Unknown symbol: '{}' at {}", identifier_expression->name, identifier_expression->source_location));
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeAssignmentExpression(
    AssignmentExpression* assignment_expression)
{
    const auto lhs = AnalyzeExpression(assignment_expression->target.get());
    if(!lhs)
    {
        return Return(lhs.error());
    }

    if(const auto* index_access = dynamic_cast<IndexAccess*>(assignment_expression->target.get()); index_access)
    {
        if(index_access->array_expr->type_info == PrimitiveType::String.get())
        {
            return Return("Strings are immutable and do not support index assignment");
        }

    }

    auto rhs = AnalyzeExpression(assignment_expression->value.get());
    if(!rhs)
    {
        return Return(std::format("Unable to parse right hand side of assignment, {}", rhs.error()));
    }

    if(!rhs.value()->IsAssignableTo(lhs.value()))
    {
        return Return(std::format(
            "Type mismatch: Cannot assign '{}' to '{}' at {}",
            rhs.value()->GetName(), lhs.value()->GetName(), assignment_expression->source_location)
        );
    }
    assignment_expression->type_info = rhs.value();
    return rhs.value();
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeCallExpression(CallExpression* call_expression)
{
    if(const auto method_call = dynamic_cast<PropertyAccess*>(call_expression->callee.get()))
    {
        auto obj = AnalyzeExpression(method_call->object_expr.get());
        if(!obj) return std::unexpected(obj.error());
        
        if(const auto* mod_type = dynamic_cast<const ModuleType*>(obj.value()))
        {
            const std::string mangled = Mangling::ModuleSymbol(mod_type->GetName(), method_call->property_name);
            if(const auto func_type = symbol_table.LookupVariable(mangled))
            {
                call_expression->callee = std::make_unique<IdentifierExpression>(mangled, method_call->source_location);
                call_expression->callee->type_info = func_type.value().type;
            }
            else if(const auto struct_type = symbol_table.LookupType(mangled))
            {
                call_expression->callee = std::make_unique<IdentifierExpression>(mangled, method_call->source_location);
                call_expression->callee->type_info = struct_type;
            }
            else
            {
                return Return(std::format("Module '{}' does not have member '{}' at {}", mod_type->GetName(), method_call->property_name, method_call->source_location));
            }
        }
        else
        {
            const std::string method_name = MangleMethodName(method_call->property_name, obj.value());
            auto func_type = symbol_table.LookupVariable(method_name);
            const Type* fn_ptr = func_type ? func_type.value().type : obj.value()->GetMethod(method_call->property_name);
            if(!fn_ptr)
            {
                return Return(
                    std::format("Struct '{}' does not have method '{}' at {}", obj.value()->GetName(), method_call->property_name, method_call->source_location)
                );
            }

            call_expression->arguments.insert(
                call_expression->arguments.begin(),
                std::move(method_call->object_expr)
            );

            call_expression->callee = std::make_unique<IdentifierExpression>(
                method_name,
                method_call->source_location
            );
            call_expression->callee->type_info = fn_ptr;
        }
    }

    const auto callable = AnalyzeExpression(call_expression->callee.get());
    if(!callable)
    {
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

        for(size_t i = 0; i < call_expression->arguments.size(); ++i)
        {
            const auto* param = func_type->GetParameters()[i];
            auto& arg = call_expression->arguments[i];

            auto arg_type = AnalyzeExpression(arg.get());
            if(!arg_type)
            {
                return Return(std::format(
                    "Error parsing function '{}' parameter expression, {}",
                    function_name, arg_type.error()));
            }

            const bool is_param_printable = (param->GetName() == constants::PRINTABLE_INTERFACE || param->GetName() == std::format("std::{}", constants::PRINTABLE_INTERFACE));
            const bool is_param_string = (param == PrimitiveType::String.get());

            if(is_param_printable || (is_param_string && arg_type.value() != PrimitiveType::String.get()))
            {
                const std::string to_string_mangled = MangleMethodName(std::string(constants::TO_STRING_METHOD), arg_type.value());
                if(!symbol_table.LookupVariable(to_string_mangled))
                {
                    return Return(std::format(
                        "Error calling function '{}': Type '{}' does not implement interface '{}' (missing '{}()' method) at {}",
                        function_name, arg_type.value()->GetName(), constants::PRINTABLE_INTERFACE, constants::TO_STRING_METHOD, arg->source_location));
                }

                const auto loc = arg->source_location;
                auto prop_access = std::make_unique<PropertyAccess>(std::move(arg), std::string(constants::TO_STRING_METHOD), loc);
                auto to_string_call = std::make_unique<CallExpression>(std::move(prop_access), std::vector<std::unique_ptr<Expression>>{}, loc);

                auto res = AnalyzeCallExpression(to_string_call.get());
                if(!res)
                {
                    return std::unexpected(res.error());
                }

                arg = std::move(to_string_call);
                arg_type = res;
            }

            if(!arg_type.value()->IsAssignableTo(param))
            {
                return Return(
                    std::format("Error calling function '{}': Type '{}' is not assignable to '{}' at {}",
                    function_name, arg_type.value()->GetName(), param->GetName(), arg->source_location));
            }

            if(dynamic_cast<const AnyType*>(param) && !dynamic_cast<const AnyType*>(arg_type.value()))
            {
                Token any_tok{TokenType::Identifier, "any", arg->source_location};
                auto loc = arg->source_location;
                auto implicit_cast = std::make_unique<CastExpression>(std::move(arg), any_tok, loc);
                implicit_cast->type_info = AnyType::Instance.get();
                arg = std::move(implicit_cast);
            }
        }

        call_expression->type_info = func_type->GetReturnType();
        return func_type->GetReturnType();
    }
    else if(const auto* struct_type = dynamic_cast<const StructType*>(callable.value()))
    {
        if(call_expression->arguments.empty())
        {
            call_expression->type_info = struct_type; call_expression->is_constructor_call = true;
            return struct_type;
        }

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

            call_expression->type_info = struct_type; call_expression->is_constructor_call = true;
        }
        return struct_type;
    }

    return Return("Attempted to call a value that is not a function or struct");
}

void SemanticAnalyzer::RegisterBinaryOperator(const TokenType op, const Type* left, const Type* right, const Type* result)
{
    binary_operators[{op, left, right}] = result;
}

const Type* SemanticAnalyzer::LookupBinaryOperator(const TokenType op, const Type* left, const Type* right) const
{
    if(dynamic_cast<const EnumType*>(left) && left == right)
    {
        if(op == TokenType::Equal || op == TokenType::NotEqual)
        {
            return PrimitiveType::Bool.get();
        }
    }

    if(const auto it = binary_operators.find({op, left, right}); it != binary_operators.end())
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
    if(const auto it = unary_operators.find({op, operand}); it != unary_operators.end())
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
    symbol_table.DefineType(PrimitiveType::Char->GetName(), PrimitiveType::Char.get());
    symbol_table.DefineType(PrimitiveType::Void->GetName(), PrimitiveType::Void.get());
    symbol_table.DefineType(AnyType::Instance->GetName(), AnyType::Instance.get());

    const Type* int_t = PrimitiveType::Int.get();
    const Type* float_t = PrimitiveType::Float.get();
    const Type* bool_t = PrimitiveType::Bool.get();
    const Type* string_t = PrimitiveType::String.get();
    const Type* char_t = PrimitiveType::Char.get();

    // Arithmetic
    for(const auto op : {TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash})
    {
        RegisterBinaryOperator(op, int_t, int_t, int_t);
        RegisterBinaryOperator(op, float_t, float_t, float_t);
    }

    RegisterBinaryOperator(TokenType::Modulo, int_t, int_t, int_t);

    // String concatenation
    RegisterBinaryOperator(TokenType::Plus, string_t, string_t, string_t);

    // Logical (bool)
    for(const auto op : {TokenType::OrOr, TokenType::AndAnd})
    {
        RegisterBinaryOperator(op, bool_t, bool_t, bool_t);
    }

    // Comparison
    for(const auto op : {TokenType::Less, TokenType::LessEqual, TokenType::Greater, TokenType::GreaterEqual})
    {
        RegisterBinaryOperator(op, int_t, int_t, bool_t);
        RegisterBinaryOperator(op, float_t, float_t, bool_t);
        RegisterBinaryOperator(op, int_t, float_t, bool_t);
        RegisterBinaryOperator(op, float_t, int_t, bool_t);
        RegisterBinaryOperator(op, char_t, char_t, bool_t);
    }

    // Equality
    for(const auto op : {TokenType::Equal, TokenType::NotEqual})
    {
        RegisterBinaryOperator(op, int_t, int_t, bool_t);
        RegisterBinaryOperator(op, float_t, float_t, bool_t);
        RegisterBinaryOperator(op, bool_t, bool_t, bool_t);
        RegisterBinaryOperator(op, string_t, string_t, bool_t);
        RegisterBinaryOperator(op, char_t, char_t, bool_t);
    }

    RegisterUnaryOperator(TokenType::Negate, bool_t, bool_t);

    RegisterUnaryOperator(TokenType::Minus, int_t, int_t);
    RegisterUnaryOperator(TokenType::Minus, float_t, float_t);
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeIndexAccess(IndexAccess* index_access)
{
    auto indexed_type = AnalyzeExpression(index_access->array_expr.get());
    if(!indexed_type)
    {
        return std::unexpected(indexed_type.error());
    }

    auto accessing_type = AnalyzeExpression(index_access->index_expr.get());
    if(!accessing_type) return std::unexpected(accessing_type.error());
    if(accessing_type.value() != PrimitiveType::Int.get())
    {
        return Return(std::format("Indexing value must be an integer at {}", index_access->index_expr->source_location));
    }

    if(indexed_type.value() == PrimitiveType::String.get())
    {
        index_access->type_info = PrimitiveType::Char.get();
        return index_access->type_info;
    }
    if(auto* array_t = dynamic_cast<const ArrayType*>(indexed_type.value()))
    {
        index_access->type_info = array_t->GetElementType();
        return index_access->type_info;
    }

    return Return("Cannot index into a non-array/string type");
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeArrayLiteral(ArrayLiteral* array_node)
{
    if(array_node->elements.empty())
    {
        return Return("Cannot infer type of empty array literal");
    }

    auto first_type = AnalyzeExpression(array_node->elements[0].get());
    if(!first_type)
    {
        return std::unexpected(first_type.error());
    }

    for(size_t i = 1; i < array_node->elements.size(); ++i)
    {
        auto elem_type = AnalyzeExpression(array_node->elements[i].get());
        if(!elem_type) return std::unexpected(elem_type.error());

        if(elem_type.value() != first_type.value())
        {
            return Return(std::format(
                "Array literal elements must all be of the same type, expected type '{}' got type '{}' at {}",
                first_type.value()->GetName(), elem_type.value()->GetName(), array_node->elements[i]->source_location));
        }
    }

    allocated_types.push_back(std::make_unique<ArrayType>(first_type.value()));
    array_node->type_info = allocated_types.back().get();
    return array_node->type_info;
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzePropertyAccess(PropertyAccess* property_access)
{
    // check beforehand if it's an enum as that cant be analyzed as an expression
    if(auto* id_expr = dynamic_cast<IdentifierExpression*>(property_access->object_expr.get()))
    {
        if(auto* type = symbol_table.LookupType(id_expr->name))
        {
            if(auto* enum_type = dynamic_cast<const EnumType*>(type))
            {
                if(!enum_type->Get(property_access->property_name))
                {
                    return std::unexpected(std::format("Enum '{}' does not have variant '{}'", id_expr->name, property_access->property_name));
                }

                // cache the int value to use in the compiler
                property_access->cached_enum_value = enum_type->Get(property_access->property_name);
                 property_access->type_info = enum_type; // but dont treat it as an int to prevent implicit casts
                return property_access->type_info;
            }
        }
    }

    auto lhs = AnalyzeExpression(property_access->object_expr.get());
    if(!lhs) return std::unexpected(lhs.error());

    if(dynamic_cast<const ArrayType*>(lhs.value()) || lhs.value() == PrimitiveType::String.get())
    {
        if(property_access->property_name == "length")
        {
            property_access->type_info = PrimitiveType::Int.get();
            return property_access->type_info;
        }
        return Return(std::format("Type '{}' only has a 'length' property", lhs.value()->GetName()));
    }

    if(auto* mod_type = dynamic_cast<const ModuleType*>(lhs.value()))
    {
        const std::string mangled = Mangling::ModuleSymbol(mod_type->GetName(), property_access->property_name);
        if(const auto& identifier = symbol_table.LookupVariable(mangled))
        {
            property_access->type_info = identifier->type;
            return identifier->type;
        }
        return Return(std::format("Module '{}' does not export '{}'", mod_type->GetName(), property_access->property_name));
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

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeSwitchExpression(SwitchExpression* switch_expression)
{
    auto switched_variable = AnalyzeExpression(switch_expression->target.get());
    if(!switched_variable) return switched_variable;

    // currently (maybe changed later) can only do switch (primitive_t) maybe down the road will include any type with operator ==
    if(
        !IsIn(
            switched_variable.value(),
            PrimitiveType::Bool.get(), PrimitiveType::Int.get(), PrimitiveType::Float.get(),
            PrimitiveType::Char.get(), PrimitiveType::String.get()
        )
        && !dynamic_cast<const EnumType*>(switched_variable.value())
    )
    {
        return Return(std::format(
            "Calling switch on non primitive type at {}", switch_expression->target->source_location)
        );
    }

    const Type* return_type = nullptr;
    for(const auto& [pattern, result]: switch_expression->branches)
    {
        if(pattern)
        {
            auto pattern_t = AnalyzeExpression(pattern.get());
            if(!pattern_t) return pattern_t;

            if(!switched_variable.value()->IsAssignableTo(pattern_t.value()))
            {
                return Return(std::format(
                    "In switch expression, can not match pattern type '{}' to type '{}' at {}",
                    pattern_t.value()->GetName(), switched_variable.value()->GetName(), pattern->source_location));
            }
        }

        auto result_t = AnalyzeExpression(result.get());
        if(!result_t) return result_t;

        if(!return_type) return_type = result_t.value();
        if(!return_type->IsAssignableTo(result_t.value()))
        {
            return Return(std::format(
                "In switch expression, can not return type '{}' to type '{}' at {}",
                result_t.value()->GetName(), return_type->GetName(), result->source_location));
        }
    }

    switch_expression->type_info = return_type;
    return return_type;
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeInterfaceDeclaration(
        InterfaceDeclaration* interface_declaration)
{
    if (analysis_pass == AnalysisPass::Validation) return {};

    std::vector<std::pair<std::string, const FunctionType*>> expected_methods;
    expected_methods.reserve(interface_declaration->methods.size());

    // if(symbol_table.LookupVariable()) // allow redefinition?
    for(auto& [name, parameters, return_type, return_type_info]: interface_declaration->methods)
    {
        const Type* cur_return_type = PrimitiveType::Void.get();
        if(return_type.has_value())
        {
            cur_return_type = symbol_table.LookupType(return_type->lexeme);
            if(!cur_return_type)
            {
                return Return(std::format(
                    "Unknown return type '{}' for interface method '{}'",
                    return_type->lexeme, name.lexeme)
                );
            }
            if(dynamic_cast<const InterfaceType*>(cur_return_type))
            {
                return Return(std::format(
                    "Cannot use interface '{}' as a concrete return type for interface method '{}' at {}",
                    return_type->lexeme, name.lexeme, return_type->source_location)
                );
            }
        }
        return_type_info = cur_return_type;

        std::vector<const Type*> param_types;
        param_types.reserve(parameters.size());

        for(auto& [name, type_name, type_info]: parameters)
        {
            const auto* param_type = symbol_table.LookupType(type_name.lexeme);
            if(!param_type)
            {
                return Return(std::format(
                    "Unknown parameter type '{}' for interface method '{}'",
                    type_name.lexeme, name.lexeme)
                );
            }
            if(dynamic_cast<const InterfaceType*>(param_type))
            {
                return Return(std::format(
                    "Cannot use interface '{}' as a concrete type for parameter '{}' of interface method '{}' at {}",
                    type_name.lexeme, name.lexeme, name.lexeme, type_name.source_location)
                );
            }
            if(param_type == PrimitiveType::Void.get())
            {
                return Return(
                    std::format("Cannot use 'void' as a type for parameter '{}' of interface method '{}' at {}",
                        name.lexeme, name.lexeme, type_name.source_location)
                );
            }
            type_info = param_type;
            param_types.push_back(param_type);
        }

        auto func_type = std::make_unique<FunctionType>(std::move(param_types), cur_return_type);
        expected_methods.emplace_back(name.lexeme, func_type.get());
        allocated_types.push_back(std::move(func_type));
    }

    allocated_types.push_back(std::make_unique<InterfaceType>(interface_declaration->name.lexeme, std::move(expected_methods)));
    symbol_table.DefineType(interface_declaration->name.lexeme, allocated_types.back().get());

    return {};

}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeForLoop(const ForLoop* for_loop)
{
    auto iterable_type = AnalyzeExpression(for_loop->iterable.get());
    if(!iterable_type) return std::unexpected(iterable_type.error());




    auto& iterator = for_loop->iterator_name;
    const Type* iter_var_type = PrimitiveType::Int.get();

    if(auto* struct_type = dynamic_cast<const StructType*>(iterable_type.value()))
    {
        if(const auto* next_method = struct_type->GetMethod(std::string(constants::NEXT_METHOD)))
        {
            iter_var_type = next_method->GetReturnType();
        }
    }
    else if(auto* array_type = dynamic_cast<const ArrayType*>(iterable_type.value()))
    {
        iter_var_type = array_type->GetElementType();
    }
    else if(iterable_type.value() == PrimitiveType::String.get())
    {
        iter_var_type = PrimitiveType::Char.get();
    }
    else
    {
        return Return("Iterable must be a struct type (extending Iterable), array, or string");
    }

    symbol_table.PushScope();
    symbol_table.DefineVariable({iterator.lexeme, iter_var_type});

    ++loop_depth;
    if(auto body = AnalyzeStatement(for_loop->body.get());
        !body)
    {
        return std::unexpected(body.error());
    }
    --loop_depth;

    symbol_table.PopScope();
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::AnalyzeExtendStatement(const ExtendStatement* extend_statement)
{
    auto* type = symbol_table.LookupType(extend_statement->target_struct.lexeme);
    if(!type)
    {
        type = symbol_table.LookupType(MangleName(extend_statement->target_struct.lexeme));
    }
    if(!type)
    {
        return Return(std::format(
            "Unknown type '{}' in extend statement at {}",
            extend_statement->target_struct.lexeme, extend_statement->source_location));
    }

    const auto* interface_type = symbol_table.LookupType(extend_statement->interface_extending.lexeme);
    if(!interface_type)
    {
        interface_type = symbol_table.LookupType(MangleName(extend_statement->interface_extending.lexeme));
    }
    if(!interface_type)
    {
        return Return(std::format(
            "Unknown interface '{}' at {}", extend_statement->interface_extending.lexeme, extend_statement->source_location));
    }

    auto* iface = dynamic_cast<const InterfaceType*>(interface_type);
    if(!iface)
    {
        return std::unexpected(std::format(
            "Type '{}' is not an interface at {}",
            extend_statement->interface_extending.lexeme, extend_statement->interface_extending.source_location)
        );
    }
    pending_interface_checks.emplace_back(
        type,
        iface,
        extend_statement->source_location
    );

    const_cast<Type*>(type)->AddImplementedInterface(iface);
    return {};
}

std::expected<void, std::string> SemanticAnalyzer::EnsureInterfacesImplemented() const
{
    for (const auto& check : pending_interface_checks)
    {
        for (const auto& [method_name, function] : check.interface_type->GetExpectedMethods())
        {
            const auto* implemented = check.type->GetMethod(method_name);
            if(!implemented)
            {
                return Return(std::format(
                    "Type '{}' does not implement interface '{}' method '{}'",
                    check.type->GetName(), check.interface_type->GetName(), method_name
                ));
            }

            const auto* expected_func_type = function;

            if(!implemented->GetReturnType()->IsAssignableTo(expected_func_type->GetReturnType()))
            {
                return Return(std::format(
                    "Method '{}' on type '{}' has return type '{}' but interface '{}' expects '{}'",
                    method_name, check.type->GetName(),
                    implemented->GetReturnType()->GetName(),
                    check.interface_type->GetName(),
                    expected_func_type->GetReturnType()->GetName()
                ));
            }

            const auto& impl_params = implemented->GetParameters();
            const auto& expected_params = expected_func_type->GetParameters();

            // impl params has the self at the start
            if(impl_params.size() != expected_params.size() + 1)
            {
                return Return(std::format(
                    "Method '{}' on type '{}' has {} parameters, but interface '{}' expects {}",
                    method_name, check.type->GetName(),
                    impl_params.size() - 1, check.interface_type->GetName(), expected_params.size()
                ));
            }

            for (size_t i = 0; i < expected_params.size(); ++i)
            {
                // again... i + 1 because i is the self parameter
                if (impl_params[i + 1] != expected_params[i])
                {
                    return Return(std::format(
                        "Parameter {} of method '{}' on type '{}' has type '{}', but interface '{}' expects '{}'",
                        i + 1, method_name, check.type->GetName(),
                        impl_params[i + 1]->GetName(), check.interface_type->GetName(), expected_params[i]->GetName()
                    ));
                }
            }
        }
    }
    return {};
}


std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeCastExpression(CastExpression* cast_expression)
{
    auto left = AnalyzeExpression(cast_expression->left.get());
    if(!left) return left;
    const Type* left_type = left.value();
    const Type* right_type = symbol_table.LookupType(cast_expression->type_name.lexeme);
    if(!right_type)
    {
        return std::unexpected(std::format(
            "Error in cast expression at {}: unknown typename '{}'",
            cast_expression->source_location, cast_expression->type_name.lexeme)
        );
    }
    cast_expression->type_info = right_type;
    const bool is_left_any = dynamic_cast<const AnyType*>(left_type) != nullptr;
    const bool is_right_any = dynamic_cast<const AnyType*>(right_type) != nullptr;

    // upcasting to any always allowed, downcasting is checked by vm,
    // third check redudant, but i guess no reason to not have, allowing var x = 4 as int;
    if(is_left_any || is_right_any || left_type == right_type)
    {
        return right_type;
    }
    return std::unexpected(std::format(
        "Invalid cast at {}: Cannot cast '{}' to '{}'",
        cast_expression->source_location, left_type->GetName(), right_type->GetName())
    );
}

std::expected<const Type*, std::string> SemanticAnalyzer::AnalyzeIsExpression(IsExpression* is_expression)
{
    auto left = AnalyzeExpression(is_expression->left.get());
    if(!left) return left;

    const Type* right_type = symbol_table.LookupType(is_expression->type_name.lexeme);
    if(!right_type)
    {
        return std::unexpected(std::format(
            "Error in 'is' expression at {}: unknown typename '{}'",
            is_expression->source_location, is_expression->type_name.lexeme)
        );
    }
    is_expression->target_type = right_type;
    is_expression->type_info = PrimitiveType::Bool.get();
    return PrimitiveType::Bool.get();
}