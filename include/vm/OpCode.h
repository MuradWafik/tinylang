#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_CALL,
    OP_NIL,
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
        case OpCode::OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
        case OpCode::OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OpCode::OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OpCode::OP_CALL: return "OP_CALL";
        case OpCode::OP_NIL: return "OP_NIL";
        default: return "Unknown";
    }
}