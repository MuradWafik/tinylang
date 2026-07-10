#include "Type.h"

#include <algorithm>
#include <print>
#include <ranges>


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

const Type* PrimitiveType::GetBinaryOperatorResult(const Token& op, const Type* right_type) const
{
    const Type* types[] = { this, right_type };


    // lambda to check for all the types to see if they match
    auto is_primitive = [](PrimitiveKind kind) {
        return [kind](const Type* t) {
            auto* prim = dynamic_cast<const PrimitiveType*>(t);
            return prim && prim->GetKind() == kind;
        };
    };

    if(op.IsEqualityOperator() &&  is_primitive(this->kind)(right_type))
    {
        return Bool.get();
    }

    for(const auto& [primitive_kind, primitive_type]:
        std::initializer_list<std::pair<PrimitiveKind, const Type*>>{{PrimitiveKind::Float, Float.get()}, {PrimitiveKind::Int, Int.get()}}
    )
    {
        if (std::ranges::all_of(types, is_primitive(primitive_kind))) {
            if(op.IsArithmeticOperator())
            {
                return primitive_type;
            }
            if(op.IsComparisonOperator())
            {
                return Bool.get();
            }
        }
    }

    // bools only support logical and / logical or  (and equality)
    if (std::ranges::all_of(types, is_primitive(PrimitiveKind::Bool))) {
        if (op.IsLogicalOperator() || op.IsEqualityOperator()) return Bool.get();
    }

    // strings only support equality operators
    if (std::ranges::all_of(types, is_primitive(PrimitiveKind::String))) {
        if (op.IsEqualityOperator()) return Bool.get();
    }



    return nullptr;
}

const Type* PrimitiveType::GetUnaryOperatorResult(const Token& op) const
{
    return nullptr;
}
