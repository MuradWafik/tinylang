#include "vm/VM.h"
#include <print>

#include "interpreter/TreeWalkInterpreter.h"

InterpretResult VM::Interpret(Chunk* chunk)
{
    this->chunk = chunk;
    this->ip = chunk->code.data();
    return Run();
}

InterpretResult VM::Run()
{
    while(true)
    {
        switch(auto opcode = static_cast<OpCode>(*ip++))
        {
            case OpCode::OP_RETURN: return InterpretResult::INTERPRET_OK;
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
            default: return InterpretResult::INTERPRET_COMPILE_ERROR;
        }
    }

    // TODO: Implement the VM execution loop
    return InterpretResult::INTERPRET_OK;
}

void VM::Push(RuntimeValue value)
{
    stack.push(std::move(value));
}

RuntimeValue VM::Pop()
{
    auto value = std::move(stack.top());
    stack.pop();
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
        if constexpr(std::is_same_v<T2, int> && std::is_same_v<T3, int>
            || std::is_same_v<T2, float> && std::is_same_v<T3, float>
            || std::is_same_v<T2, std::string> && std::is_same_v<T3, std::string>)
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
    const uint8_t index = *ip++; // constant index is on the next instruction
    return chunk->constants[index]; // value can maybe be cleared? maybe take by reference and move
}

void VM::GetGlobal()
{
    const auto symbol_name = Pop();
    Push(globals[std::get<std::string>(symbol_name)]);

}

void VM::SetGlobal()
{
    const auto symbol_name = Pop();
    const auto value = Pop();
    globals[std::get<std::string>(symbol_name)] = value;
}
