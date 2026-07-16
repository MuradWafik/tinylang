#include "analysis/Type.h"

bool PrimitiveType::IsIntegral() const
{
    switch(kind)
    {
        case PrimitiveKind::Int:
        case PrimitiveKind::Float:
            return true;
        default: return false;
    }
}

bool FunctionType::IsAssignableTo(const Type* other) const
{
    return this == other;
}

std::string FunctionType::GetName() const
{
    std::string name = "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        name += arguments[i]->GetName();
        if (i + 1 < arguments.size()) {
            name += ", ";
        }
    }
    name += ") -> " + return_type->GetName();
    return name;
}
