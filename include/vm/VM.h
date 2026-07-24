#pragma once
// No longer need stack header
#include <unordered_map>
#include <vector>

#include "vm/RuntimeValue.h"
#include "vm/Chunk.h"
#include "vm/PluginLoader.h"

enum class InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM {
public:
    VM() = default;

    InterpretResult Interpret(Chunk* chunk);

    void LoadNativeFunction();

    std::optional<RuntimeValue> GetGlobal(const std::string& name) const {
        if (auto it = globals.find(name); it != globals.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::vector<RuntimeValue> stack{};
    std::unordered_map<std::string, RuntimeValue> globals{};
    PluginLoader plugin_loader{};

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
    RuntimeValue HandleGreaterThan();
    RuntimeValue HandleGreaterEqualThan();
    RuntimeValue HandleLessThan();
    RuntimeValue HandleLessEqualThan();
    RuntimeValue HandleEqualTo();
    RuntimeValue HandleNotEqualTo();

    void DefineGlobal();
    void GetGlobal();
    void SetGlobal();
    void CallFunction();
    void HandleJump();
    void SetLocalVariable();
    void HandleJumpIfFalse();
    void GetLocalVariable();
    void HandleLoop();
    void HandleJumpIfFalsePeek();
    void HandleJumpIfTruePeek();

    InterpretResult Run();
    RuntimeValue& ExtractNextConstant();

    template<typename T0, typename T1, typename Target>
    static constexpr bool AreBoth()
    {
        return std::is_same_v<T0, Target> && std::is_same_v<T1, Target>;
    }

};
