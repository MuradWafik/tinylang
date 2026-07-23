#pragma once
#include <cstdint>

enum class OpCode : uint8_t
{
    // --- Data & Variables ---
    OP_CONSTANT,      // [1 byte operand: constant_index] | Stack: Pushes constants[index]
    OP_NIL,           // [No operands]                    | Stack: Pushes std::monostate{}
    
    // --- Globals ---
    OP_DEFINE_GLOBAL, // [1 byte operand: name_index]     | Stack: Pops value. Defines global variable
    OP_GET_GLOBAL,    // [1 byte operand: name_index]     | Stack: Pushes value of global variable
    OP_SET_GLOBAL,    // [1 byte operand: name_index]     | Stack: Peeks value and sets global variable

    // --- Locals ---
    OP_GET_LOCAL,     // [1 byte operand: local_index]    | Stack: Pushes value from stack_base + 1 + index
    OP_SET_LOCAL,     // [1 byte operand: local_index]    | Stack: Peeks value and copies to stack_base + 1 + index

    // --- Math ---
    OP_ADD,           // [No operands]                    | Stack: Pops right, pops left, pushes (left + right)
    OP_SUBTRACT,      // [No operands]                    | Stack: Pops right, pops left, pushes (left - right)
    OP_MULTIPLY,      // [No operands]                    | Stack: Pops right, pops left, pushes (left * right)
    OP_DIVIDE,        // [No operands]                    | Stack: Pops right, pops left, pushes (left / right)
    OP_NEGATE,        // [No operands]                    | Stack: Pops value, pushes (-value)

    // --- Comparisons ---
    OP_EQUAL,         // [No operands]                    | Stack: Pops right, pops left, pushes (left == right)
    OP_NOT_EQUAL,     // [No operands]                    | Stack: Pops right, pops left, pushes (left != right)
    OP_GREATER,       // [No operands]                    | Stack: Pops right, pops left, pushes (left > right)
    OP_GREATER_EQUAL, // [No operands]                    | Stack: Pops right, pops left, pushes (left >= right)
    OP_LESS,          // [No operands]                    | Stack: Pops right, pops left, pushes (left < right)
    OP_LESS_EQUAL,    // [No operands]                    | Stack: Pops right, pops left, pushes (left <= right)

    // --- Control Flow ---
    OP_POP,                // [No operands]                    | Stack: Pops value and discards it
    OP_JUMP_IF_FALSE,      // [2 byte operand: jump_offset]    | Stack: Pops condition. If false, advances IP by offset
    OP_JUMP_IF_FALSE_PEEK, // [2 byte operand: jump_offset]    | Stack: Peeks condition. If false, advances IP by offset
    OP_JUMP_IF_TRUE_PEEK,  // [2 byte operand: jump_offset]    | Stack: Peeks condition. If true, advances IP by offset
    OP_JUMP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally advances IP by offset
    OP_LOOP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally rewinds IP backwards by offset

    // --- Functions ---
    OP_CALL,               // [1 byte operand: num_args]       | Stack: Peeks function at top - num_args, creates CallFrame
    OP_RETURN,             // [No operands]                    | Stack: Pops return_value, erases frame, pushes return_value
};



constexpr std::string OpCodeToString(const OpCode op_code)
{
    switch(op_code)
    {
        case OpCode::OP_CONSTANT: return "OP_CONSTANT";
        case OpCode::OP_NIL: return "OP_NIL";
        case OpCode::OP_ADD: return "OP_ADD";
        case OpCode::OP_SUBTRACT: return "OP_SUBTRACT";
        case OpCode::OP_MULTIPLY: return "OP_MULTIPLY";
        case OpCode::OP_DIVIDE: return "OP_DIVIDE";
        case OpCode::OP_NEGATE: return "OP_NEGATE";
        case OpCode::OP_RETURN: return "OP_RETURN";
        case OpCode::OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
        case OpCode::OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OpCode::OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OpCode::OP_GET_LOCAL: return "OP_GET_LOCAL";
        case OpCode::OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OpCode::OP_POP: return "OP_POP";
        case OpCode::OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OpCode::OP_JUMP_IF_FALSE_PEEK: return "OP_JUMP_IF_FALSE_PEEK";
        case OpCode::OP_JUMP_IF_TRUE_PEEK: return "OP_JUMP_IF_TRUE_PEEK";
        case OpCode::OP_JUMP: return "OP_JUMP";
        case OpCode::OP_LOOP: return "OP_LOOP";
        case OpCode::OP_CALL: return "OP_CALL";
        case OpCode::OP_EQUAL: return "OP_EQUAL";
        case OpCode::OP_NOT_EQUAL: return "OP_NOT_EQUAL";
        case OpCode::OP_GREATER: return "OP_GREATER";
        case OpCode::OP_GREATER_EQUAL: return "OP_GREATER_EQUAL";
        case OpCode::OP_LESS: return "OP_LESS";
        case OpCode::OP_LESS_EQUAL: return "OP_LESS_EQUAL";
        default: return "Unknown";
    }
}