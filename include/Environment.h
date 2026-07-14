#pragma once
#include <list>
#include <memory>
#include <unordered_map>

#include "RuntimeValue.h"
#include "StringHash.h"

class Environment
{
public:
    explicit Environment(std::shared_ptr<Environment> parent) : enclosing{std::move(parent)} {}
    RuntimeValue Get(std::string_view name);
    void Assign(const std::string& name, const RuntimeValue& value);
    void Define(const std::string& name, const RuntimeValue& value);

    std::shared_ptr<Environment> enclosing;
    // Functions should be able to reference their scope and global scope,
    // but if a function calls another, it shouldnt see that functions vriables
private:
    std::unordered_map<std::string, RuntimeValue, StringHash, std::equal_to<>> scoped_variables;
};

