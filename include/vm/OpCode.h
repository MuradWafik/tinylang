#pragma once
#include <cstdint>

enum class OpCode : uint8_t {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN
};



constexpr std::string OpCodeToString(const OpCode op_code)
{
    switch(op_code)
    {
        case OpCode::OP_CONSTANT: return "OP_CONSTANT";
        case OpCode::OP_ADD: return "OP_ADD";
        case OpCode::OP_SUBTRACT: return "OP_SUBTRACT";
        case OpCode::OP_MULTIPLY: return "OP_MULTIPLY";
        case OpCode::OP_DIVIDE: return "OP_DIVIDE";
        case OpCode::OP_NEGATE: return "OP_NEGATE";
        case OpCode::OP_RETURN: return "OP_RETURN";
        default: return "Unknown";
    }
}