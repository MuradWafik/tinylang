#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "frontend/Statement.h" // exportable statement



struct Namespace
{
    std::string name; // "math"

    // All ASTs that declared this namespace
    std::vector<std::unique_ptr<ASTNode>> asts;

    // The combined exports from ALL files in this namespace
    std::unordered_map<std::string, ExportableStatement*> exports;
};

class ModuleRegistry 
{


public:
    ModuleRegistry() = default;

    void RegisterModule(const std::string& module_name, std::vector<std::unique_ptr<ASTNode>> asts);
    Namespace* GetNamespace(const std::string& module_name);
    const auto& GetNamespaces() const { return namespaces; }

    void RegisterExport(const std::string& module_name, const std::string& symbol_name, ExportableStatement* statement);
    
    // helper for the Semantic Analyzer to quickly check exports
    ExportableStatement* GetExport(const std::string& module_name, const std::string& symbol_name);
private:
    // namespace string to the combined Namespace object
    std::unordered_map<std::string, std::unique_ptr<Namespace>> namespaces;
};