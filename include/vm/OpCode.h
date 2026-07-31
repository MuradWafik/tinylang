#pragma once
#include <cstdint>

#include <magic_enum.hpp>

enum class OpCode : uint8_t
{
    // --- Data & Variables ---
    OP_CONSTANT_INT,      // [1 byte operand: constant_index] | Stack: Pushes 4 byte int from constants[index]
    OP_CONSTANT_FLOAT,    // [1 byte operand: constant_index] | Stack: Pushes 4 byte float from constants[index]
    OP_CONSTANT_BOOL,
    OP_CONSTANT_CHAR,     // [1 byte operand: constant_index] | Stack: Pushes 1 byte bool from constants[index]
    OP_CONSTANT_FUNCTION, // [1 byte operand: constant_index] | Stack: Pushes 8 byte FunctionObject* from constants[index]
    
    // --- Heap Objects ---
    OP_ALLOCATE_STRING,   // [1 byte operand: constant_index] | Stack: Reads string from constants[index], allocates StringObject on heap, pushes 8 byte Object*
    OP_ADD_STRING,        // [No operands]                    | Stack: Pops two 8 byte Object* strings, allocates new concatenated StringObject, pushes 8 byte Object*
    OP_ALLOCATE_ARRAY,    // [2 bytes: element_count] [1 byte: stride] | Stack: Pops (count*stride) bytes, allocates ArrayObject, pushes 8 byte Object*
    OP_ALLOCATE_STRUCT,   // [2 bytes: total_size] [1 byte: from_stack] | Stack: Pops 'total_size' bytes (if from_stack is true), allocates StructObject on heap, pushes 8 byte Object*
    OP_GET_PROPERTY,      // [2 bytes: byte_offset] [1 byte: size]     | Stack: Pops 8 byte StructObject*, pushes 'size' bytes from offset
    OP_SET_PROPERTY,      // [2 bytes: byte_offset] [1 byte: size]     | Stack: Pops 'size' bytes, pops 8 byte StructObject*, writes bytes, pushes bytes back
    OP_GET_LENGTH,        // [No operands]                             | Stack: Pops 8 byte Object*, pushes 4 byte int
    OP_GET_INDEX,         // [1 byte: stride]                          | Stack: Pops 4 byte int index, pops 8 byte ArrayObject*, pushes 'stride' bytes
    OP_SET_INDEX,         // [1 byte: stride]                          | Stack: Pops 'stride' bytes, pops 4 byte int index, pops 8 byte ArrayObject*, writes bytes, pushes bytes back
    OP_GET_STRING_CHAR,   // [No operands]                             | Stack: Pops 4 byte int index, pops 8 byte Object*, pushes 1 byte char

    // --- Globals ---
    OP_DEFINE_GLOBAL, // [1 byte operand: name_index] [1 byte operand: size] | Stack: Pops 'size' bytes. Defines global variable
    OP_GET_GLOBAL,    // [1 byte operand: name_index] [1 byte operand: size] | Stack: Pushes 'size' bytes of global variable
    OP_SET_GLOBAL,    // [1 byte operand: name_index] [1 byte operand: size] | Stack: Peeks 'size' bytes and sets global variable

    // --- Locals ---
    OP_GET_LOCAL,     // [2 byte operand: byte_offset] [1 byte operand: size] | Stack: Pushes 'size' bytes from frame_base + offset
    OP_SET_LOCAL,     // [2 byte operand: byte_offset] [1 byte operand: size] | Stack: Peeks 'size' bytes and copies to frame_base + offset

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
    OP_MOD_INT,           // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 4 bytes (left % right)
    OP_NOT_BOOL,          // [No operands] | Stack: Pops 1 byte, pushes 1 byte (!value)

    // --- Comparisons ---
    OP_EQUAL_INT,         // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left == right)
    OP_EQUAL_FLOAT,       // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left == right)
    OP_EQUAL_BOOL,        // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left == right)
    OP_EQUAL_CHAR,        // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left == right)
    OP_EQUAL_STRING,      // [No operands] | Stack: Pops 8 bytes (right), pops 8 bytes (left), pushes 1 byte bool (left == right)
    OP_NOT_EQUAL_INT,     // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_FLOAT,   // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_BOOL,    // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_CHAR,    // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left != right)
    OP_NOT_EQUAL_STRING,  // [No operands] | Stack: Pops 8 bytes (right), pops 8 bytes (left), pushes 1 byte bool (left != right)
    OP_GREATER_INT,       // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left > right)
    OP_GREATER_FLOAT,     // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left > right)
    OP_GREATER_CHAR,      // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left > right)
    OP_GREATER_EQUAL_INT, // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left >= right)
    OP_GREATER_EQUAL_FLOAT, // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left >= right)
    OP_GREATER_EQUAL_CHAR,  // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left >= right)
    OP_LESS_INT,          // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left < right)
    OP_LESS_FLOAT,        // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left < right)
    OP_LESS_CHAR,         // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left < right)
    OP_LESS_EQUAL_INT,    // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left <= right)
    OP_LESS_EQUAL_FLOAT,  // [No operands] | Stack: Pops 4 bytes (right), pops 4 bytes (left), pushes 1 byte bool (left <= right)
    OP_LESS_EQUAL_CHAR,   // [No operands] | Stack: Pops 1 byte (right), pops 1 byte (left), pushes 1 byte bool (left <= right)

    // --- Control Flow ---
    OP_POP,                // [1 byte operand: size]           | Pops 'size' bytes
    OP_JUMP_IF_FALSE,      // [2 byte operand: jump_offset]    | Stack: Pops condition. If false, advances IP by offset
    OP_JUMP_IF_FALSE_PEEK, // [2 byte operand: jump_offset]    | Stack: Peeks condition. If false, advances IP by offset
    OP_JUMP_IF_TRUE_PEEK,  // [2 byte operand: jump_offset]    | Stack: Peeks condition. If true, advances IP by offset
    OP_JUMP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally advances IP by offset
    OP_LOOP,               // [2 byte operand: jump_offset]    | Stack: Unconditionally rewinds IP backwards by offset

    // --- Functions ---
    OP_LOAD_NATIVE,        // [3 byte operand: path_index, name_index, num_args] | Stack: Loads DLL, extracts function, saves as global
    OP_CALL,               // [1 byte operand: num_args]       | Stack: Peeks function at top - num_args, creates CallFrame
    OP_RETURN,             // [1 byte operand: size]           | Stack: Pops 'size' bytes return_value, erases frame, pushes 'size' bytes return_value
};


inline std::string OpCodeToString(const OpCode op_code)
{
    return std::string(magic_enum::enum_name(op_code));
}
