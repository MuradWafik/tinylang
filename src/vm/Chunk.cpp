#include "vm/Chunk.h"
#include <print>
void Chunk::Write(const uint8_t byte, const uint32_t line)
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::Write(OpCode opcode, const uint32_t line)
{
    Write(static_cast<uint8_t>(opcode), line);
}

size_t Chunk::AddConstant(ConstantValue value)
{
    constants.push_back(std::move(value));
    return constants.size() - 1;
}


int Chunk::DisassembleInstruction(int offset) const
{
    std::print("{:04} ", offset);
    if (offset > 0 && lines[offset] == lines[offset - 1])
    {
        std::print("   | ");
    }
    else
    {
        std::print("{:4} ", lines[offset]);
    }

    const auto opcode = static_cast<OpCode>(code[offset]);
    const auto opcode_name = OpCodeToString(opcode);

    switch(opcode)
    {
        case OpCode::OP_CONSTANT_INT:
        case OpCode::OP_CONSTANT_FLOAT:
        case OpCode::OP_CONSTANT_BOOL:
        case OpCode::OP_CONSTANT_FUNCTION:
        case OpCode::OP_ALLOCATE_STRING:

        case OpCode::OP_DEFINE_GLOBAL_INT:
        case OpCode::OP_DEFINE_GLOBAL_FLOAT:
        case OpCode::OP_DEFINE_GLOBAL_BOOL:
        case OpCode::OP_DEFINE_GLOBAL_FUNCTION:
        case OpCode::OP_GET_GLOBAL_INT:
        case OpCode::OP_GET_GLOBAL_FLOAT:
        case OpCode::OP_GET_GLOBAL_BOOL:
        case OpCode::OP_GET_GLOBAL_FUNCTION:
        case OpCode::OP_SET_GLOBAL_INT:
        case OpCode::OP_SET_GLOBAL_FLOAT:
        case OpCode::OP_SET_GLOBAL_BOOL:
        {
            const auto index = code[offset + 1];
            const auto& variable = constants[index];
            std::println("{:<24} {:4} '{}'", opcode_name, index, variable);
            return offset + 2;
        }
        case OpCode::OP_GET_LOCAL_INT:
        case OpCode::OP_GET_LOCAL_FLOAT:
        case OpCode::OP_GET_LOCAL_BOOL:
        case OpCode::OP_SET_LOCAL_INT:
        case OpCode::OP_SET_LOCAL_FLOAT:
        case OpCode::OP_SET_LOCAL_BOOL:
        {
            const uint16_t local_index = (code[offset + 1] << 8) | code[offset + 2];
            std::println("{:<24} {:4} (byte offset)", opcode_name, local_index);
            return offset + 3;
        }
        case OpCode::OP_ALLOCATE_STRUCT:
        {
            const uint16_t size = (code[offset + 1] << 8) | code[offset + 2];
            std::println("{:<24} {:4} (bytes)", opcode_name, size);
            return offset + 3;
        }
        case OpCode::OP_ALLOCATE_ARRAY:
        {
            const uint16_t count = (code[offset + 1] << 8) | code[offset + 2];
            const uint8_t stride = code[offset + 3];
            std::println("{:<24} {:4} (count) {:4} (stride)", opcode_name, count, stride);
            return offset + 4;
        }
        case OpCode::OP_GET_PROPERTY:
        case OpCode::OP_SET_PROPERTY:
        {
            const uint16_t byte_offset = (code[offset + 1] << 8) | code[offset + 2];
            const uint8_t size = code[offset + 3];
            std::println("{:<24} {:4} (offset) {:4} (size)", opcode_name, byte_offset, size);
            return offset + 4;
        }
        case OpCode::OP_GET_INDEX:
        case OpCode::OP_SET_INDEX:
        {
            const uint8_t stride = code[offset + 1];
            std::println("{:<24} {:4} (stride)", opcode_name, stride);
            return offset + 2;
        }
        case OpCode::OP_JUMP_IF_FALSE:
        case OpCode::OP_JUMP:
        case OpCode::OP_LOOP:
        case OpCode::OP_JUMP_IF_FALSE_PEEK:
        case OpCode::OP_JUMP_IF_TRUE_PEEK:
        {
            const uint16_t jump = (code[offset + 1] << 8) | code[offset + 2];
            std::println("{:<24} {:4}", opcode_name, jump);
            return offset + 3;
        }
        case OpCode::OP_LOAD_NATIVE:
        {
            const auto path_index = code[offset + 1];
            const auto name_index = code[offset + 2];
            const auto num_args   = code[offset + 3];
            const auto return_bytes = code[offset + 4];
            const auto& path = constants[path_index];
            const auto& name = constants[name_index];
            std::println("{:<24} path_idx: {:2} '{}', name_idx: {:2} '{}', args: {:2}, ret_bytes: {:2}", 
                opcode_name, path_index, path, name_index, name, num_args, return_bytes);
            return offset + 5;
        }
        case OpCode::OP_CALL:
        {
            const uint16_t arg_bytes = (code[offset + 1] << 8) | code[offset + 2];
            std::println("{:<24} {:4} arg bytes", opcode_name, arg_bytes);
            return offset + 3;
        }
        default:
        {
            std::println("{}", opcode_name);
            return offset + 1;
        }
    }
}

void Chunk::Disassemble(std::string_view name) const
{
    std::println("=== {} ===", name);
    for(int offset = 0; offset < code.size();)
    {
        offset = DisassembleInstruction(offset);
    }
}