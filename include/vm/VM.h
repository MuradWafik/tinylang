#pragma once
// No longer need stack header
#include <unordered_map>
#include <vector>
#include <iostream>

#include "utils/Utils.h"
#include "vm/ConstantValue.h"
#include "vm/Chunk.h"
#include "vm/PluginLoader.h"
#include "vm/VMHeap.h"

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

    InterpretResult StartProgram(const std::unordered_map<std::string, std::unique_ptr<Chunk>>& chunks, const std::vector<std::string>& ordered_modules);
    InterpretResult Interpret(Chunk* chunk);

    void SetLocal();

    template <typename T>
    T GetGlobal(const uint16_t offset) const
    {
        T val;
        std::memcpy(&val, globals.data() + offset, sizeof(T));
        return val;
    }

private:
    std::vector<uint8_t> stack{};
public:
    const std::vector<uint8_t>& GetGlobals() const { return globals; }
    void SetProjectRoot(std::filesystem::path root) { project_root = std::move(root); }
private:
    std::filesystem::path project_root{};
    std::vector<uint8_t> globals{};
    PluginLoader plugin_loader{};
    std::vector<std::unique_ptr<FunctionObject>> allocated_native_functions;
    // functions are no longer shared ptrs to work with the raw bytes so need to be managed

    struct CallFrame
    {
        Chunk* chunk;
        uint8_t* ip;
        size_t stack_base;
    };

    std::vector<CallFrame> call_frames{};
    VMHeap heap;

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

    void DefineGlobal(const uint16_t offset, const uint8_t size)
    {
        if(offset + size > globals.size())
        {
            globals.resize(offset + size);
        }
        std::memcpy(globals.data() + offset, stack.data() + stack.size() - size, size);
        stack.resize(stack.size() - size);
    }

    void GetGlobal(const uint16_t offset, const uint8_t size)
    {
        stack.insert(stack.end(), globals.data() + offset, globals.data() + offset + size);
    }

    void SetGlobal(const uint16_t offset, const uint8_t size)
    {
        std::memcpy(globals.data() + offset, stack.data() + stack.size() - size, size);
    }

    void SetLocalVariable(const uint16_t offset, const uint8_t size)
    {
        std::memcpy(
            stack.data() + call_frames.back().stack_base + sizeof(FunctionObject*) + offset,
            stack.data() + stack.size() - size, size
        );
    }

    void GetLocalVariable(const uint16_t offset, const uint8_t size)
    {
        const uint8_t* ptr = stack.data() + call_frames.back().stack_base + sizeof(FunctionObject*) + offset;
        stack.insert(stack.end(), ptr, ptr + size);
    }

    template <typename T>
    T ReadAndPopBytes(std::vector<uint8_t>& vector)
    {
        T value = ReadBytes<T>(vector);
        vector.resize(vector.size() - sizeof(T));
        return value;
    }

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

    std::optional<InterpretResult> Return(const uint8_t size)
    {
        // If it's the outermost scope, exit
        if(call_frames.size() == 1)
        {
            call_frames.pop_back();
            return InterpretResult::INTERPRET_OK;
        }

        const auto frame = call_frames.back();
        call_frames.pop_back();

        if(size > 0)
        {
            const uint8_t* return_value_ptr = stack.data() + stack.size() - size;
            std::vector<uint8_t> temp_buffer(return_value_ptr, return_value_ptr + size);
            
            stack.resize(frame.stack_base);
            stack.insert(stack.end(), temp_buffer.begin(), temp_buffer.end());
        }
        else
        {
            stack.resize(frame.stack_base);
        }

        return std::nullopt;
    }

    void HandleJump();
    void HandleLoop();
    void HandleJumpIfFalsePeek();
    void HandleJumpIfTruePeek();
    void HandleJumpIfFalse();
    void CallFunction();
    InterpretResult Run();
    ConstantValue& ExtractNextConstant();
    void AddString();
    void EqualString();
    void NotEqualString();
    void AllocateString();
    void LoadNativeFunction();
    void AllocateArray();
    void GetArrayIndex();
    void SetArrayIndex();
    void GetStringChar();
    void AllocateStruct();
    void GetProperty();
    void SetProperty();
    void GetLength();
    void GetLocal();
    void DefineGlobal();
    void Pop();
    void Duplicate();
    void BoxAny();
    void CastCheck();
    void IsCheck();

    template<typename T0, typename T1, typename Target>
    static constexpr bool AreBoth()
    {
        return std::is_same_v<T0, Target> && std::is_same_v<T1, Target>;
    }

    template <typename T, typename... Args>
    T* AllocateObject(Args&&... args)
    {
        // Check if gc needs to run before allocations
        if(heap.Size() >= heap.gc_threshold)
        {
            heap.CollectGarbage(stack, globals);
            heap.gc_threshold = heap.Size() * 2;
        }
        return heap.Allocate<T>(std::forward<Args>(args)...);
    }
};
