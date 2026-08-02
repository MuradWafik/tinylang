#include "project/ModuleRegistry.h"

void ModuleRegistry::RegisterModule(const std::string& module_name, std::vector<std::unique_ptr<ASTNode>> asts)
{
    if (!namespaces.contains(module_name))
    {
        namespaces[module_name] = std::make_unique<Namespace>();
        namespaces[module_name]->name = module_name;
    }

    // Move all ASTs from file into the namespaces
    for (auto& ast : asts)
    {
        namespaces[module_name]->asts.push_back(std::move(ast));
    }
}

Namespace* ModuleRegistry::GetNamespace(const std::string& module_name)
{
    if (namespaces.contains(module_name))
    {
        return namespaces[module_name].get();
    }
    return nullptr;
}

void ModuleRegistry::RegisterExport(const std::string& module_name, const std::string& symbol_name, ExportableStatement* statement)
{
    namespaces[module_name]->exports[symbol_name] = statement;
}

ExportableStatement* ModuleRegistry::GetExport(const std::string& module_name, const std::string& symbol_name)
{
    if (namespaces.contains(module_name))
    {
        if (auto& exports = namespaces[module_name]->exports;
            exports.contains(symbol_name))
        {
            return exports[symbol_name];
        }
    }
    return nullptr;
}
