#include "Type.h"

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