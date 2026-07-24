#pragma once
// No longer need stack header
#include <unordered_map>
#include <vector>

#include "utils/Utils.h"
#include "vm/ConstantValue.h"
#include "vm/Chunk.h"
#include "vm/PluginLoader.h"

enum class InterpretResult
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM
{
public:
    VM() = default;

    InterpretResult Interpret(Chunk* chunk);

    void LoadNativeFunction();

    std::optional<ConstantValue> GetGlobal(const std::string& name) const
    {
        if (const auto it = globals.find(name); it != globals.end())
            {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::vector<uint8_t> stack{};
    std::unordered_map<std::string, ConstantValue> globals{};
    PluginLoader plugin_loader{};

    struct CallFrame
    {
        Chunk* chunk;
        uint8_t* ip;
        size_t stack_base;
    };

    std::vector<CallFrame> call_frames{};


    template <fundamental T>
    void Push(T value)
    {
        return WriteBytes(stack, value);
    }

    template <fundamental T>
    T Pop()
    {
        return ReadAndPopBytes<T>(stack);
    }

    template <fundamental T>
    T Peek()
    {
        return ReadBytes<T>(stack);
    }


    uint8_t HandleNegate();

    template <fundamental T>
    void DefineGlobal()
    {
        const auto global_symbol = ExtractNextConstant();
        const auto value = Pop<T>();
        globals[std::get<std::string>(global_symbol)] = value;
    }

    template <fundamental T>
    void GetGlobal()
    {
        Push<T>(globals[std::get<std::string>(ExtractNextConstant())]);
    }

    template <fundamental T>
    void SetGlobal()
    {
        const auto value = Peek<T>();
        globals[std::get<std::string>(ExtractNextConstant())] = value;
    }

    void CallFunction();
    void HandleJump();

    template <fundamental T>
    void SetLocalVariable()
    {
        const auto local_index = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
        // Peek at the top of the stack, and copy it into the local slot

        const int value = ReadBytes<int32_t>(stack);
        std::memcpy(stack.data() + call_frames.back().stack_base + local_index, &value, sizeof(int));
    }

    void HandleJumpIfFalse();

    template <fundamental T>
    void VM::GetLocalVariable()
    {
        const auto local_index = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
        // stack_base is the function, so +1 jumps to the start of the variables
        const int value = ReadBytesAbsolute<int32_t>(stack, call_frames.back().stack_base + local_index);
        Push(value);
    }


    void HandleLoop();
    void HandleJumpIfFalsePeek();
    void HandleJumpIfTruePeek();


    template<fundamental T>
    T Add()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l + r;
    }

    template<fundamental T>
    T Subtract()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l - r;
    }

    template<fundamental T>
    T Multiply()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l * r;
    }

    template<fundamental T>
    T Divide()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l / r;
    }

    template<fundamental T>
    bool GreaterThan()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l > r;
    }

    template<fundamental T>
    bool GreaterEqualThan()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l >= r;
    }

    template<fundamental T>
    bool LessThan()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l < r;
    }

    template<fundamental T>
    bool LessEqualThan()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l <= r;
    }

    template<fundamental T>
    bool EqualTo()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l == r;
    }

    template<fundamental T>
    bool NotEqualTo()
    {
        auto r = Pop<T>();
        auto l = Pop<T>();
        return l != r;
    }

    template <fundamental T>
    std::optional<InterpretResult> Return()
    {
        // If it's the outermost scope, exit
        if(call_frames.size() == 1)
        {
            call_frames.pop_back();
            return InterpretResult::INTERPRET_OK;
        }

        const auto return_value = Pop<T>();
        const auto frame = call_frames.back();
        call_frames.pop_back();
        // Erase everything (the function pointer, arguments, and locals)
        stack.resize(frame.stack_base);

        Push(return_value);
        return std::nullopt;
    }

    // void
    std::optional<InterpretResult> Return()
    {
        // If it's the outermost scope, exit
        if (call_frames.size() == 1)
        {
            call_frames.pop_back();
            return InterpretResult::INTERPRET_OK;
        }
        // grab the frame so we know where its base is
        const auto frame = call_frames.back();
        call_frames.pop_back();
        // Erase everything (the function pointer, arguments, and locals)
        stack.resize(frame.stack_base);
        return std::nullopt;
    }

    InterpretResult Run();
    ConstantValue& ExtractNextConstant();

    template<typename T0, typename T1, typename Target>
    static constexpr bool AreBoth()
    {
        return std::is_same_v<T0, Target> && std::is_same_v<T1, Target>;
    }
};
