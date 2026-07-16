#pragma once
#include <stack>
#include <vector>

#include "interpreter/RuntimeValue.h"
#include "vm/Chunk.h"

enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM {
public:
    VM() = default;

    InterpretResult Interpret(Chunk* chunk);

private:
    std::stack<RuntimeValue, std::vector<RuntimeValue>> stack;
    Chunk* chunk;
    uint8_t* ip;

    void Push(RuntimeValue value);
    RuntimeValue Pop();
    InterpretResult Run();
};
