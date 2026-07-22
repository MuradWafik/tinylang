#pragma once
// No longer need stack header
#include <unordered_map>
#include <vector>

#include "vm/RuntimeValue.h"
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
    std::vector<RuntimeValue> stack{};
    std::unordered_map<std::string, RuntimeValue> globals{};

    struct CallFrame
    {
        Chunk* chunk;
        uint8_t* ip;
        size_t stack_base;
    };

    std::vector<CallFrame> call_frames{};


    void Push(RuntimeValue value);
    RuntimeValue Pop();
    RuntimeValue HandleAdd();
    RuntimeValue HandleSubtract();
    RuntimeValue HandleMultiply();
    RuntimeValue HandleDivide();
    RuntimeValue HandleNegate();
    void DefineGlobal();
    void GetGlobal();
    void SetGlobal();
    void CallFunction();

    InterpretResult Run();
    RuntimeValue& ExtractNextConstant();

    template<typename T0, typename T1, typename Target>
    static constexpr bool AreBoth()
    {
        return std::is_same_v<T0, Target> && std::is_same_v<T1, Target>;
    }

};
