#pragma once
#include <vector>
#include <cstdint>
#include "vm/OpCode.h"
#include "interpreter/RuntimeValue.h"

class Chunk {
public:
    std::vector<uint8_t> code;
    std::vector<RuntimeValue> constants;
    std::vector<int> lines;

    Chunk() = default;

    void Write(uint8_t byte, int line);
    void Write(OpCode opcode, int line);
    int AddConstant(RuntimeValue value);

     void Disassemble(std::string_view name);
};


