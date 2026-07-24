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
        case OpCode::OP_CONSTANT:
        case OpCode::OP_DEFINE_GLOBAL:
        case OpCode::OP_GET_GLOBAL:
        case OpCode::OP_SET_GLOBAL:
        {
            const auto index = code[offset + 1];
            const auto& variable = constants[index];
            std::println("{:<16} {:4} '{}'", opcode_name, index, variable);
            return offset + 2;
        }
        case OpCode::OP_LOAD_NATIVE:
        {
            const auto path_index = code[offset + 1];
            const auto name_index = code[offset + 2];
            const auto num_args   = code[offset + 3];
            const auto& path = constants[path_index];
            const auto& name = constants[name_index];
            std::println("{:<16} {:4} path: '{}', {:4} name: '{}', {:4} args", opcode_name, path_index, path, name_index, name, num_args);
            return offset + 4;
        }
        case OpCode::OP_CALL:
        {
            const auto num_args = code[offset + 1];
            std::println("{:<16} {:4} args", opcode_name, num_args);
            return offset + 2;
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