#include "vm/VM.h"

#include <cassert>
#include <print>

InterpretResult VM::Interpret(Chunk* chunk)
{
    call_frames.emplace_back(chunk, chunk->code.data(), 0);
    return Run();
}

#define DEBUG_TRACE_EXECUTION

InterpretResult VM::Run()
{
    while(true)
    {
#ifdef DEBUG_TRACE_EXECUTION
        std::print("          ");
        for (const auto& slot : stack) {
            std::print("[ {} ]", slot);
        }
        std::println("");
        
        const auto& frame = call_frames.back();
        frame.chunk->DisassembleInstruction(frame.ip - frame.chunk->code.data());
#endif

        switch(static_cast<OpCode>(*(call_frames.back().ip)++))
        {
            case OpCode::OP_RETURN:
            {
                // exit if its outermost scope, program terminates
                if(call_frames.size() == 1)
                {
                    call_frames.pop_back();

                    return InterpretResult::INTERPRET_OK;
                }

                const auto return_value = Pop();

                const auto frame = call_frames.back();
                call_frames.pop_back();

                // Erase everything that belongs to this function;
                // deleting the function object, the arguments, and any temp variables.
                stack.erase(stack.begin() + frame.stack_base, stack.end());

                Push(return_value);
                break;
            }
            case OpCode::OP_CONSTANT:
            {
                Push(std::move(ExtractNextConstant()));
                break;
            }
            case OpCode::OP_ADD:
            {
                Push(std::move(HandleAdd()));
                break;
            }
            case OpCode::OP_SUBTRACT:
            {
                Push(std::move(HandleSubtract()));
                break;
            }
            case OpCode::OP_MULTIPLY:
            {
                Push(std::move(HandleMultiply()));
                break;
            }
            case OpCode::OP_DIVIDE:
            {
                Push(std::move(HandleDivide()));
            }
            case OpCode::OP_NEGATE:
            {
                Push(std::move(HandleNegate()));
            }
            case OpCode::OP_DEFINE_GLOBAL:
            {
                DefineGlobal();
                break;
            }
            case OpCode::OP_GET_GLOBAL:
            {
                GetGlobal();
                break;
            }
            case OpCode::OP_SET_GLOBAL:
            {
                SetGlobal();
                break;
            }
            case OpCode::OP_CALL:
            {
                CallFunction();
                break;
            }
            case OpCode::OP_NIL:
            {
                Push(std::monostate{});
                break;
            }
            default: return InterpretResult::INTERPRET_COMPILE_ERROR;
        }
    }
    return InterpretResult::INTERPRET_OK;
}

void VM::Push(RuntimeValue value)
{
    stack.push_back(std::move(value));
}

RuntimeValue VM::Pop()
{
    auto value = std::move(stack.back());
    stack.pop_back();
    return value;
}

RuntimeValue VM::HandleAdd()
{
    auto right = Pop();
    auto left = Pop();

    return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> RuntimeValue {
        using T2 = std::decay_t<T0>;
        using T3 = std::decay_t<T1>;

        // direct int, and float math, along with string concatenation
        if constexpr(
               AreBoth<T2, T3, int>()
            || AreBoth<T2, T3, float>()
            || AreBoth<T2, T3, std::string>())
        {
            return l + r;
        }

        // allow for int and float addition, casting to float
        else if constexpr(std::is_same_v<T2, int> && std::is_same_v<T3, float>)
        {
            return static_cast<float>(l) + r;
        }
        else if constexpr(std::is_same_v<T2, float> && std::is_same_v<T3, int>)
        {
            return l + static_cast<float>(r);
        }


        return std::monostate{};
    }, left, right);
}

void VM::DefineGlobal()
{
    const auto global_symbol = ExtractNextConstant();
    const auto value = Pop();
    globals[std::get<std::string>(global_symbol)] = value;
}

RuntimeValue& VM::ExtractNextConstant()
{
    const uint8_t index = *(call_frames.back().ip)++; // constant index is on the next instruction
    return call_frames.back().chunk->constants[index]; // value can maybe be cleared? maybe take by reference and move
}

void VM::GetGlobal()
{
    Push(globals[std::get<std::string>(ExtractNextConstant())]);
}

void VM::SetGlobal()
{
    const auto value = Pop();
    globals[std::get<std::string>(ExtractNextConstant())] = value;
}

void VM::CallFunction()
{
    // Assumes everything is correctly ordered
    // OP_CALL NUM_ARGS
    const auto num_args = *(call_frames.back().ip)++;


    // cant pop since the function itself and its args must remain in scope
    // for recursion and referencing the args
    const auto func_val = stack[stack.size() - 1 - num_args];
    const auto function_object = std::get<std::shared_ptr<FunctionObject>>(func_val);

    assert(function_object->num_args == num_args && "Function called with too much arguments");

    call_frames.emplace_back(
        function_object->chunk.get(),
        function_object->chunk->code.data(),
        stack.size() - 1 - num_args
    );
}

RuntimeValue VM::HandleSubtract()
{
    auto right = Pop();
    auto left = Pop();

    return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> RuntimeValue {
        using T2 = std::decay_t<T0>;
        using T3 = std::decay_t<T1>;

        // direct int, and float math
        if constexpr(
               AreBoth<T2, T3, int>()
            || AreBoth<T2, T3, float>())
        {
            return l - r;
        }

        // allow for int and float subtraction, casting to float
        else if constexpr(std::is_same_v<T2, int> && std::is_same_v<T3, float>)
        {
            return static_cast<float>(l) - r;
        }
        else if constexpr(std::is_same_v<T2, float> && std::is_same_v<T3, int>)
        {
            return l - static_cast<float>(r);
        }

        return std::monostate{};
    }, left, right);
}

RuntimeValue VM::HandleMultiply()
{
    auto right = Pop();
    auto left = Pop();

    return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> RuntimeValue {
        using T2 = std::decay_t<T0>;
        using T3 = std::decay_t<T1>;

        // direct int, and float math
        if constexpr(
               AreBoth<T2, T3, int>()
            || AreBoth<T2, T3, float>())
        {
            return l * r;
        }

        // allow for int and float multiplication, casting to float
        else if constexpr(std::is_same_v<T2, int> && std::is_same_v<T3, float>)
        {
            return static_cast<float>(l) * r;
        }
        else if constexpr(std::is_same_v<T2, float> && std::is_same_v<T3, int>)
        {
            return l * static_cast<float>(r);
        }

        return std::monostate{};
    }, left, right);
}

RuntimeValue VM::HandleDivide()
{
    auto right = Pop();
    auto left = Pop();

    return std::visit([]<typename T0, typename T1>(T0&& l, T1&& r) -> RuntimeValue {
        using T2 = std::decay_t<T0>;
        using T3 = std::decay_t<T1>;

        // direct int, and float math
        if constexpr(
               AreBoth<T2, T3, int>()
            || AreBoth<T2, T3, float>())
        {
            return l/r;
        }

        // allow for int and float division, casting to float
        else if constexpr(std::is_same_v<T2, int> && std::is_same_v<T3, float>)
        {
            return static_cast<float>(l) / r;
        }
        else if constexpr(std::is_same_v<T2, float> && std::is_same_v<T3, int>)
        {
            return l / static_cast<float>(r);
        }

        return std::monostate{};
    }, left, right);
}

RuntimeValue VM::HandleNegate()
{
    auto var = Pop();

    return std::visit([]<typename T>(T&& v) -> RuntimeValue {
        using T1 = std::decay_t<T>;
        if constexpr (std::is_same_v<T1, bool>)
        {
            return !v;
        }
        else if constexpr (std::is_same_v<T1, int> || std::is_same_v<T1, float>)
        {
            return -v;
        }

        return std::monostate{};
    }, var);
}