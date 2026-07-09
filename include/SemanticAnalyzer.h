#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ASTNode.h"
#include "Statement.h"

struct Symbol {
    std::string name;
    const Type* type; // non-owning
};

struct StringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }
    size_t operator()(const char* s) const {
        return std::hash<std::string_view>{}(s);
    }
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
private:
    SymbolTable symbol_table{};

    std::expected<std::string, std::string> AnalyzeNode(ASTNode* node);
    std::expected<std::string, std::string> AnalyzeVariableDeclaration(VariableDeclaration* variable_declaration);

};
