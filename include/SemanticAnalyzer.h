#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ASTNode.h"
#include "Statement.h"
#include "StringHash.h"
#include "Token.h"
#include "Type.h"

struct OperatorSignature {
    TokenType op;
    const Type* left;
    const Type* right;

    bool operator==(const OperatorSignature& other) const {
        return op == other.op && left == other.left && right == other.right;
    }
};

struct OperatorSignatureHash {
    size_t operator()(const OperatorSignature& sig) const {
        const size_t h1 = std::hash<std::underlying_type_t<TokenType>>{}(static_cast<int>(sig.op));
        const size_t h2 = std::hash<const Type*>{}(sig.left);
        const size_t h3 = std::hash<const Type*>{}(sig.right);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};


struct UnaryOperatorSignature {
    TokenType op;
    const Type* operand;

    bool operator==(const UnaryOperatorSignature& other) const {
        return op == other.op && operand == other.operand;
    }
};


struct UnaryOperatorSignatureHash {
    size_t operator()(const UnaryOperatorSignature& sig) const {
        const size_t h1 = std::hash<int>{}(static_cast<std::underlying_type_t<TokenType>>(sig.op));
        const size_t h2 = std::hash<const Type*>{}(sig.operand);
        return h1 ^ (h2 << 1);
    }
};



struct Symbol {
    std::string name;
    const Type* type; // non-owning
};


struct Scope {
    std::unordered_map<std::string, Symbol, StringHash, std::equal_to<>> variables;
    std::unordered_map<std::string, const Type*, StringHash, std::equal_to<>> types;
};

class SymbolTable {
public:
    inline void PushScope()
    {
        scopes.emplace_back();
    }
    
    inline void PopScope()
    {
        scopes.pop_back();
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
class SemanticAnalyzer {
public:
    std::expected<void, std::string> Analyze(std::vector<std::unique_ptr<ASTNode>>& program);
    std::expected<void, std::string> Analyze(ASTNode* node);
    const Type* LookupBinaryOperator(TokenType op, const Type* left, const Type* right) const;
    const Type* LookupUnaryOperator(TokenType op, const Type* operand) const;

private:
    SymbolTable symbol_table{};
    size_t loop_depth{0};
    std::unordered_map<OperatorSignature, const Type*, OperatorSignatureHash> binary_operators;
    std::unordered_map<UnaryOperatorSignature, const Type*, UnaryOperatorSignatureHash> unary_operators;

    // Has to take ownership of the types since they must be dynamically allocated for dynamic dispatch
    std::vector<std::unique_ptr<Type>> allocated_types;

    const Type* current_function_return_type{nullptr}; // used to track if the return statement type matches
    // as return statements have no information of their functions


    std::expected<void, std::string> AnalyzeNode(ASTNode* node);
    std::expected<void, std::string> AnalyzeStatement(Statement* stmt);
    std::expected<const Type*, std::string> AnalyzeExpression(Expression* expr);

    std::expected<void, std::string> AnalyzeVariableDeclaration(const VariableDeclaration* variable_declaration);
    std::expected<void, std::string> AnalyzeIfStatement(const IfStatement* if_statement);
    std::expected<void, std::string> AnalyzeWhileStatement(const WhileStatement* while_statement);
    std::expected<void, std::string> AnalyzeFunctionDeclaration(const FunctionDeclaration* function_declaration);
    std::expected<void, std::string> AnalyzeBreakStatement(const BreakStatement* break_statement);
    std::expected<void, std::string> AnalyzeContinueStatement(const ContinueStatement* continue_statement);
    std::expected<void, std::string> AnalyzeReturnStatement(const ReturnStatement* return_statement);
    std::expected<void, std::string> AnalyzeBodyStatement(const BodyStatement* body_statement);


    std::expected<const Type*, std::string> AnalyzeBinaryExpression(BinaryExpression* binary_expression);
    std::expected<const Type*, std::string> AnalyzeUnaryExpression(UnaryExpression* unary_expression);
    std::expected<const Type*, std::string> AnalyzeIdentifierExpression(IdentifierExpression* identifier_expression);
    std::expected<const Type*, std::string> AnalyzeAssignmentExpression(AssignmentExpression* assignment_expression);
    std::expected<const Type*, std::string> AnalyzeCallExpression(CallExpression* call_expression);

    void RegisterBinaryOperator(TokenType op, const Type* left, const Type* right, const Type* result);
    void RegisterUnaryOperator(TokenType op, const Type* operand, const Type* result);

    static std::unexpected<std::string> Return(const std::string_view error)
    {
        return std::unexpected(std::format("Error in semantic analysis: {}", error));
    }

    void InitializeDefaults();

};
