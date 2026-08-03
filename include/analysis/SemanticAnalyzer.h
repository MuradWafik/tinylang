#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis/Type.h"
#include "frontend/ASTNode.h"
#include "frontend/Statement.h"
#include "frontend/Token.h"
#include "project/ModuleRegistry.h"
#include "project/ProjectConfig.h"
#include "utils/Mangling.h"
#include "utils/Utils.h"

struct OperatorSignature
{
    TokenType op;
    const Type* left;
    const Type* right;

    bool operator==(const OperatorSignature& other) const {
        return op == other.op && left == other.left && right == other.right;
    }
};

struct OperatorSignatureHash
{
    size_t operator()(const OperatorSignature& sig) const
    {
        const size_t h1 = std::hash<std::underlying_type_t<TokenType>>{}(static_cast<int>(sig.op));
        const size_t h2 = std::hash<const Type*>{}(sig.left);
        const size_t h3 = std::hash<const Type*>{}(sig.right);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};


struct UnaryOperatorSignature
{
    TokenType op;
    const Type* operand;

    bool operator==(const UnaryOperatorSignature& other) const
    {
        return op == other.op && operand == other.operand;
    }
};


struct UnaryOperatorSignatureHash
{
    size_t operator()(const UnaryOperatorSignature& sig) const
    {
        const size_t h1 = std::hash<int>{}(static_cast<std::underlying_type_t<TokenType>>(sig.op));
        const size_t h2 = std::hash<const Type*>{}(sig.operand);
        return h1 ^ (h2 << 1);
    }
};

struct Symbol
{
    std::string name;
    const Type* type; // non-owning
};

struct Scope
{
    std::unordered_map<std::string, Symbol, StringHash, std::equal_to<>> variables;
    std::unordered_map<std::string, const Type*, StringHash, std::equal_to<>> types;
};

class SymbolTable
{
public:
    void PushScope()
    {
        scopes.emplace_back();
    }
    
    void PopScope()
    {
        scopes.pop_back();
    }

    [[nodiscard]] size_t GetScopeDepth() const
    {
        return scopes.size() - 1;
    }

    // Variable API
    void DefineVariable(const Symbol& symbol);
    std::optional<Symbol> LookupVariable(std::string_view name);
    [[nodiscard]] bool IsDeclaredInCurrentScope(std::string_view name) const;

    // Type API
    void DefineType(std::string_view name, const Type* type);
    const Type* LookupType(std::string_view name);

private:
    std::vector<Scope> scopes; // scopes[0] = global scope
};


// walks AST and ensures the code obeys all the semantic rules of the language
class SemanticAnalyzer
{
public:
    const Type* LookupBinaryOperator(TokenType op, const Type* left, const Type* right) const;
    const Type* LookupUnaryOperator(TokenType op, const Type* operand) const;

    std::expected<void, std::string> RunAnalysis(
            );

    // Runs Pass 1 (Exports) and Pass 2 (Typechecking) on the entire registry
    std::expected<void, std::string> AnalyzeAll();
    SemanticAnalyzer(ProjectConfig* project_config, ModuleRegistry* module_registry, const bool strict_mode = true)
    : strict_mode{strict_mode}, project_config{project_config}, module_registry{module_registry}
    {}

private:
    SymbolTable symbol_table{};
    size_t loop_depth{0};
    bool strict_mode;
    std::unordered_map<OperatorSignature, const Type*, OperatorSignatureHash> binary_operators;
    std::unordered_map<UnaryOperatorSignature, const Type*, UnaryOperatorSignatureHash> unary_operators;
    ProjectConfig* project_config; // non owning
    ModuleRegistry* module_registry;

    enum class AnalysisPass
    {
        Registration,
        Validation
    } analysis_pass;

    // Has to take ownership of the types since they must be dynamically allocated for dynamic dispatch
    std::vector<std::unique_ptr<Type>> allocated_types;

    const Type* current_function_return_type{nullptr}; // used to track if the return statement type matches
    // as return statements have no information of their functions

    std::string current_native_module{};
    std::string current_namespace{};
    // Native functions need to be aware of their native modules and to add them to symbol table

    struct PendingInterfaceCheck
    {
        const Type* type;
        const InterfaceType* interface_type;
        SourceLocation location;
    };
    std::vector<PendingInterfaceCheck> pending_interface_checks;

    const Type* ResolveType(std::string_view type_name);

    std::expected<void, std::string> AnalyzeNode(ASTNode* node);
    std::expected<void, std::string> AnalyzeStatement(Statement* stmt);
    std::expected<const Type*, std::string> AnalyzeExpression(Expression* expr);

    std::expected<void, std::string> AnalyzeVariableDeclaration(const VariableDeclaration* variable_declaration);
    std::expected<void, std::string> AnalyzeIfStatement(const IfStatement* if_statement);
    std::expected<void, std::string> AnalyzeWhileStatement(const WhileStatement* while_statement);
    std::expected<void, std::string> AnalyzeFunctionDeclaration(FunctionDeclaration* function_declaration);
    std::expected<void, std::string> AnalyzeBreakStatement(const BreakStatement* break_statement) const;
    std::expected<void, std::string> AnalyzeContinueStatement(const ContinueStatement* continue_statement) const;
    std::expected<void, std::string> AnalyzeReturnStatement(const ReturnStatement* return_statement);
    std::expected<void, std::string> AnalyzeBodyStatement(const BodyStatement* body_statement);
    std::expected<void, std::string> AnalyzeExpressionStatement(const ExpressionStatement* expression_statement);
    std::expected<void, std::string> AnalyzeNativeModuleStatement(const NativeImportStatement* native_module_statement);
    std::expected<void, std::string> AnalyzeNativeFunctionDeclaration(NativeFunctionDeclaration* native_function_declaration);
    std::expected<void, std::string> AnalyzeStructDeclaration(StructDeclaration* struct_declaration);
    std::expected<void, std::string> AnalyzeEnumDeclaration(const EnumDeclaration* enum_declaration);
    std::expected<void, std::string> AnalyzeInterfaceDeclaration(InterfaceDeclaration* interface_declaration);
    std::expected<void, std::string> AnalyzeForLoop(const ForLoop* for_loop);
    std::expected<void, std::string> AnalyzeExtendStatement(const ExtendStatement* extend_statement);


    std::expected<const Type*, std::string> AnalyzeBinaryExpression(BinaryExpression* binary_expression);
    std::expected<const Type*, std::string> AnalyzeUnaryExpression(UnaryExpression* unary_expression);
    std::expected<const Type*, std::string> AnalyzeIdentifierExpression(IdentifierExpression* identifier_expression);
    std::expected<const Type*, std::string> AnalyzeAssignmentExpression(AssignmentExpression* assignment_expression);
    std::expected<const Type*, std::string> AnalyzeCallExpression(CallExpression* call_expression);
    std::expected<const Type*, std::string> AnalyzeIndexAccess(IndexAccess* index_access);
    std::expected<const Type*, std::string> AnalyzeArrayLiteral(ArrayLiteral* array_node);
    std::expected<const Type*, std::string> AnalyzePropertyAccess(PropertyAccess* property_access);
    std::expected<const Type*, std::string> AnalyzeSwitchExpression(SwitchExpression* switch_expression);
    std::expected<const Type*, std::string> AnalyzeCastExpression(CastExpression* cast_expression);
    std::expected<const Type*, std::string> AnalyzeIsExpression(IsExpression* is_expression);

    void RegisterBinaryOperator(TokenType op, const Type* left, const Type* right, const Type* result);
    void RegisterUnaryOperator(TokenType op, const Type* operand, const Type* result);

    static std::unexpected<std::string> Return(const std::string_view error)
    {
        return std::unexpected(std::format( "{}", error));
    }


    void InitializeDefaults();
    std::expected<void, std::string> EnsureInterfacesImplemented() const;

    std::string MangleName(const std::string& name) const;

};
