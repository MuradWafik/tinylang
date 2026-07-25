#pragma once
#include <cstdint>

#include <magic_enum.hpp>

enum class OpCode : uint8_t
{
    // --- Data & Variables ---
    OP_CONSTANT_INT,      // [1 byte operand: constant_index] | Stack: Pushes 4 byte int from constants[index]
    OP_CONSTANT_FLOAT,    // [1 byte operand: constant_index] | Stack: Pushes 4 byte float from constants[index]
    OP_CONSTANT_BOOL,     // [1 byte operand: constant_index] | Stack: Pushes 1 byte bool from constants[index]
    OP_CONSTANT_FUNCTION, // [1 byte operand: constant_index] | Stack: Pushes 8 byte FunctionObject* from constants[index]
    
    // --- Heap Objects ---
    OP_ALLOCATE_STRING,   // [1 byte operand: constant_index] | Stack: Reads string from constants[index], allocates StringObject on heap, pushes 8 byte Object*
    OP_ADD_STRING,        // [No operands]                    | Stack: Pops two 8 byte Object* strings, allocates new concatenated StringObject, pushes 8 byte Object*
    OP_ALLOCATE_ARRAY,    // [2 byte operand: element_count]  | Stack: Pops 'count' elements, allocates ArrayObject on heap, pushes 8 byte Object*
    OP_ALLOCATE_STRUCT,   // [2 byte operand: field_count]    | Stack: Pops 'count' elements, allocates StructObject on heap, pushes 8 byte Object*
    OP_GET_PROPERTY,      // [1 byte operand: name_index]     | Stack: Pops 8 byte StructObject*, pushes property value
    OP_SET_PROPERTY,      // [1 byte operand: name_index]     | Stack: Pops value and 8 byte StructObject*, sets property, pushes value back
    OP_GET_INDEX,         // [No operands]                    | Stack: Pops 4 byte int index and 8 byte ArrayObject*, pushes array[index]
    OP_SET_INDEX,         // [No operands]                    | Stack: Pops value, pops 4 byte int index, pops 8 byte ArrayObject*, sets array[index], pushes value back

    // --- Globals ---
    OP_DEFINE_GLOBAL_INT, // [1 byte operand: name_index]     | Stack: Pops 4 byte int. Defines global variable
    OP_DEFINE_GLOBAL_FLOAT,
    OP_DEFINE_GLOBAL_BOOL,
    OP_DEFINE_GLOBAL_FUNCTION,
    OP_GET_GLOBAL_INT,    // [1 byte operand: name_index]     | Stack: Pushes 4 byte int of global variable
    OP_GET_GLOBAL_FLOAT,
    OP_GET_GLOBAL_BOOL,
    OP_GET_GLOBAL_FUNCTION,
    OP_SET_GLOBAL_INT,    // [1 byte operand: name_index]     | Stack: Peeks 4 byte int and sets global variable
    OP_SET_GLOBAL_FLOAT,
    OP_SET_GLOBAL_BOOL,

    // --- Locals ---
    OP_GET_LOCAL_INT,     // [2 byte operand: byte_offset] | Stack: Pushes 4 byte int from frame_base + offset
    OP_GET_LOCAL_FLOAT,   // [2 byte operand: byte_offset] | Stack: Pushes 4 byte float from frame_base + offset
    OP_GET_LOCAL_BOOL,    // [2 byte operand: byte_offset] | Stack: Pushes 1 byte bool from frame_base + offset
    OP_SET_LOCAL_INT,     // [2 byte operand: byte_offset] | Stack: Peeks 4 byte int and copies to frame_base + offset
    OP_SET_LOCAL_FLOAT,   // [2 byte operand: byte_offset] | Stack: Peeks 4 byte float and copies to frame_base + offset
    OP_SET_LOCAL_BOOL,    // [2 byte operand: byte_offset] | Stack: Peeks 1 byte bool and copies to frame_base + offset

    // --- Math ---
    OP_ADD_INT,           // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left + right)
    OP_ADD_FLOAT,         // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left + right)
    OP_SUBTRACT_INT,      // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left - right)
    OP_SUBTRACT_FLOAT,    // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left - right)
    OP_MULTIPLY_INT,      // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left * right)
    OP_MULTIPLY_FLOAT,    // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left * right)
    OP_DIVIDE_INT,        // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left / right)
    OP_DIVIDE_FLOAT,      // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left / right)
    OP_NEGATE_INT,        // [No operands] | Stack: Pops 4 bytes, pushes 4 bytes (-value)
    OP_NEGATE_FLOAT,      // [No operands] | Stack: Pops 4 bytes, pushes 4 bytes (-value)

    // --- Comparisons ---
    OP_EQUAL_INT,         // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left == right)
    OP_EQUAL_FLOAT,       // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left == right)
    OP_EQUAL_BOOL,        // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left == right)
    OP_NOT_EQUAL_INT,     // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_FLOAT,   // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_BOOL,    // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left != right)
    OP_GREATER_INT,       // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left > right)
    OP_GREATER_FLOAT,     // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left > right)
    OP_GREATER_EQUAL_INT, // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left >= right)
    OP_GREATER_EQUAL_FLOAT, // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left >= right)
    OP_LESS_INT,          // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left < right)
    OP_LESS_FLOAT,        // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left < right)
    OP_LESS_EQUAL_INT,    // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left <= right)
    OP_LESS_EQUAL_FLOAT,  // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left <= right)

    // --- Control Flow ---
    OP_POP_INT,            // Pops 4 bytes
    OP_POP_FLOAT,          // Pops 4 bytes
    OP_POP_BOOL,           // Pops 1 byte
    OP_JUMP_IF_FALSE,      // [2 byte operand: jump_offset]    | Stack: Pops condition. If false, advances IP by offset
    OP_JUMP_IF_FALSE_PEEK, // [2 byte operand: jump_offset]    | Stack: Peeks condition. If false, advances IP by offset
    OP_JUMP_IF_TRUE_PEEK,  // [2 byte operand: jump_offset]    | Stack: Peeks condition. If true, advances IP by offset
    OP_JUMP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally advances IP by offset
    OP_LOOP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally rewinds IP backwards by offset

    // --- Functions ---
    OP_LOAD_NATIVE,        // [3 byte operand: path_index, name_index, num_args] | Stack: Loads DLL, extracts function, saves as global
    OP_CALL,               // [1 byte operand: num_args]       | Stack: Peeks function at top - num_args, creates CallFrame
    OP_RETURN_INT,         // [No operands]                    | Stack: Pops 4 byte return_value, erases frame, pushes 4 byte return_value
    OP_RETURN_FLOAT,       // [No operands]                    | Stack: Pops 4 byte return_value, erases frame, pushes 4 byte return_value
    OP_RETURN_BOOL,        // [No operands]                    | Stack: Pops 1 byte return_value, erases frame, pushes 1 byte return_value
    OP_RETURN_VOID,        // [No operands]                    | Stack: Erases frame, pushes nothing
};


inline std::string OpCodeToString(const OpCode op_code)
{
    return std::string(magic_enum::enum_name(op_code));
}
