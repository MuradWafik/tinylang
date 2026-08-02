#pragma once

#include <concepts>
#include <format>
#include <memory>

#include "frontend/Token.h"

struct ASTNode
{
    SourceLocation source_location{"", 0, 0};
    virtual ~ASTNode() = default;
    [[nodiscard]] virtual std::string GetTypeString() const = 0;
};


// 1. Formatter for raw references (e.g., *node)
template <>
struct std::formatter<ASTNode> : std::formatter<std::string> {
    auto format(const ASTNode& node, format_context& ctx) const {
        return std::formatter<std::string>::format(node.GetTypeString(), ctx);
    }
};

// 2. Formatter for raw pointers (e.g., node.get())
template <typename T>
requires std::derived_from<T, ASTNode>
struct std::formatter<T*> : std::formatter<std::string> {
    auto format(const T* node, format_context& ctx) const {
        const std::string str = node ? node->GetTypeString() : "nullptr";
        return std::formatter<std::string>::format(str, ctx);
    }
};

// 3. Formatter for unique_ptrs (e.g., statements[i])
template <typename T>
requires std::derived_from<T, ASTNode>
struct std::formatter<std::unique_ptr<T>> : std::formatter<std::string> {
    auto format(const std::unique_ptr<T>& node, format_context& ctx) const {
        const std::string str = node ? node->GetTypeString() : "nullptr";
        return std::formatter<std::string>::format(str, ctx);
    }
};
