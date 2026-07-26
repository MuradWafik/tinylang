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
    std::vector<std::unique_ptr<FunctionObject>> functions;

    Chunk() = default;

    void Write(uint8_t byte, uint32_t line);
    void Write(OpCode opcode, uint32_t line);


private:
    template <typename T>
    void WriteOperand(T operand, uint32_t line)
    {
        for (int i = sizeof(T) - 1; i >= 0; --i)
        {
            code.push_back(static_cast<uint8_t>((operand >> (i * 8)) & 0xFF));
            lines.push_back(line);
        }
    }

public:
    template <typename... Args>
    void WriteInstruction(const uint32_t line, OpCode opcode, Args... operands)
    {
        code.push_back(static_cast<uint8_t>(opcode));
        lines.push_back(line);
        (WriteOperand(operands, line), ...);
    }

    unsigned long AddConstant(ConstantValue value);

     void Disassemble(std::string_view name) const;
    int DisassembleInstruction(int offset) const;
};


