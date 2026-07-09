#include "SemanticAnalyzer.h"

#include <ranges>

#include "Expression.h"
#include "Statement.h"
#include "Type.h"


std::expected<void, std::string> SemanticAnalyzer::Analyze(std::vector<std::unique_ptr<ASTNode>>& program)
{
    // Initialize global scope
    symbol_table.PushScope();

    // define the global types
    symbol_table.DefineType(PrimitiveType::Int->GetName(), PrimitiveType::Int.get());
    symbol_table.DefineType(PrimitiveType::Float->GetName(), PrimitiveType::Float.get());
    symbol_table.DefineType(PrimitiveType::Bool->GetName(), PrimitiveType::Bool.get());
    symbol_table.DefineType(PrimitiveType::String->GetName(), PrimitiveType::String.get());
    symbol_table.DefineType(PrimitiveType::Void->GetName(), PrimitiveType::Void.get());

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


std::expected<std::string, std::string> SemanticAnalyzer::AnalyzeNode(ASTNode* node)
{
    if(auto* var_decl = dynamic_cast<VariableDeclaration*>(node))
    {
        return AnalyzeVariableDeclaration(var_decl);
    }
    // if(auto* if_stmt = dynamic_cast<IfStatement*>(node))
    // {
    //     return AnalyzeIfStatement(if_stmt);
    // }
    // if(auto* binary_expr = dynamic_cast<BinaryExpression*>(node))
    // {
    //     return AnalyzeBinaryExpression(binary_expr);
    // }
    if(dynamic_cast<BoolLiteral*>(node)) return PrimitiveType::Bool->GetName(); // Literals return their type
    if(dynamic_cast<IntegerLiteral*>(node)) return PrimitiveType::Int->GetName();
    if(dynamic_cast<FloatLiteral*>(node)) return PrimitiveType::Float->GetName();
    if(dynamic_cast<StringLiteral*>(node)) return PrimitiveType::String->GetName();

    return std::unexpected("Unknown AST Node type");
}

std::expected<std::string, std::string> SemanticAnalyzer::AnalyzeVariableDeclaration(VariableDeclaration* variable_declaration)
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

    auto expression_type = AnalyzeNode(variable_declaration->initializer.get());
    if(!expression_type)
    {
        return std::unexpected(expression_type.error());
    }

    if (!symbol_table.LookupType(expression_type.value())->IsAssignableTo(type))
    {
        return std::unexpected(
            std::format("Unable to assign {} to type {}", expression_type.value(), type->GetName())
        );
    }

    return "void";
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

