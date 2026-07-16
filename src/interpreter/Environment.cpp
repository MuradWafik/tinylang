#include "interpreter/Environment.h"


void Environment::Assign(const std::string& name, const RuntimeValue& value)
{
    if(scoped_variables.contains(name))
    {
        scoped_variables[name] = value;
        return;
    }
    if(enclosing != nullptr)
    {
        enclosing->Assign(name, value);
        return;
    }

    throw std::runtime_error("Trying to assign unknown variable, should not reach this point!!");
}

void Environment::Define(const std::string& name, const RuntimeValue& value)
{
    scoped_variables[name] = value;
}

RuntimeValue Environment::Get(const std::string_view name)
{
    if(const auto found = scoped_variables.find(name); found != scoped_variables.end())
    {
        return found->second;
    }
    if(enclosing == nullptr)
    {
        return std::monostate{};
    }

    return enclosing->Get(name);
}
