#include "vm/Chunk.h"

#include <print>

void Chunk::Write(const uint8_t byte, const int line)
{
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::Write(OpCode opcode, const int line)
{
    Write(static_cast<uint8_t>(opcode), line);
}

int Chunk::AddConstant(RuntimeValue value)
{
    constants.push_back(std::move(value));
    return constants.size() - 1;
}


void Chunk::Disassemble(std::string_view name)
{
    std::println("=== {} ===", name);
    for(size_t i = 0; i < code.size(); ++i)
    {
        // Print the byte offset index
        std::print("{:04} ", i);

        // Print the line number (or | if it's the same line as the previous byte)
        if (i > 0 && lines[i] == lines[i - 1])
        {
            std::print("   | ");
        }
        else
        {
            std::print("{:4} ", lines[i]);
        }

        const auto opcode_int = code[i];
        const OpCode opcode{opcode_int};
        const auto opcode_name = OpCodeToString(opcode);

        if(opcode == OpCode::OP_CONSTANT)
        {
            // The operand is the next byte,  preincrement i to advance to it before accessing
            const auto variable_index = code[++i];
            const auto variable = constants[variable_index];
            std::println("{:<16} {:4} '{}'", opcode_name, variable_index, variable);
        }
        else
        {
            std::println("{}", opcode_name);
        }
    }
}