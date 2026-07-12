#include "Environment.h"


void Environment::Assign(const std::string& name, const RuntimeValue& value)
{
    if(scoped_variables.contains(name))
    {
        scoped_variables[name] = value;
    }

    if(enclosing != nullptr)
    {
        enclosing->Assign(name, value);
    }

    scoped_variables[name] = value;
}

RuntimeValue Environment::Get(const std::string_view name)
{
    if(scoped_variables.contains(name))
    {
        return scoped_variables.at(name);
    }

    if(enclosing == nullptr)
    {
        return std::monostate{};
    }

    return enclosing->Get(name);
}
