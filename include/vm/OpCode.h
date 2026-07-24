#pragma once
#include <cstdint>

#include <magic_enum.hpp>

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
    OP_LOAD_NATIVE,        // [3 byte operand: path_index, name_index, num_args] | Stack: Loads DLL, extracts function, saves as global
    OP_CALL,               // [1 byte operand: num_args]       | Stack: Peeks function at top - num_args, creates CallFrame
    OP_RETURN,             // [No operands]                    | Stack: Pops return_value, erases frame, pushes return_value
};




inline std::string OpCodeToString(const OpCode op_code)
{
    return std::string(magic_enum::enum_name(op_code));
}