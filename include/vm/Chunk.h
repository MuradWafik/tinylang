#pragma once
#include <cstdint>
#include <vector>

#include "vm/ConstantValue.h"
#include "vm/OpCode.h"

class Chunk
{
public:
    std::vector<uint8_t> code;
    std::vector<ConstantValue> constants;
    std::vector<uint32_t> lines;

    Chunk() = default;

    void Write(uint8_t byte, uint32_t line);
    void Write(OpCode opcode, uint32_t line);


    // Allows to write the OpCode with all its args instead of one by one
    template <typename... Args>
    void WriteInstruction(const uint32_t line, OpCode opcode, Args... operands)
    {
        code.push_back(static_cast<uint8_t>(opcode));
        lines.push_back(line);

        // AI GENERATED -- LOOK FURTHER INTO *FOLD EXPRESSION*
        ((code.push_back(static_cast<uint8_t>(operands)), lines.push_back(line)), ...);
    }

    unsigned long AddConstant(ConstantValue value);

     void Disassemble(std::string_view name) const;
    int DisassembleInstruction(int offset) const;
};


