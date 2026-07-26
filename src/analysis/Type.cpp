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
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        name += arguments[i]->GetName();
        if (i + 1 < arguments.size())
        {
            name += ", ";
        }
    }
    name += ") -> " + return_type->GetName();
    return name;
}

std::string ArrayType::GetName() const
{
    if(element_type != nullptr)
    {
        return this->element_type->GetName() + "[]";
    }
    return "Unknown array type";
}

bool ArrayType::IsAssignableTo(const Type* other) const
{
    if(this == other) return true;
    
    if(const auto* other_array = dynamic_cast<const ArrayType*>(other))
    {
        return this->element_type == other_array->element_type;
    }
    return false;
}


std::string StructType::GetName() const
{
    std::string result = "Fields(";
    for(const auto& [name, type]: fields)
    {
        result += std::format("{}: {} ", name, type->GetName());
    }
    result += ")";
    return result;
}

const Type* StructType::GetFieldType(const std::string_view name) const
{
    for(const auto& [field_name, type]: fields)
    {
        if(field_name == name)
        {
            return type;
        }
    }

    return nullptr;
}

bool StructType::IsAssignableTo(const Type* other) const
{
    return this == other;
}

