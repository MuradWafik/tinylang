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
        switch(*ip++)
        {
            case static_cast<uint8_t>(OpCode::OP_RETURN): return InterpretResult::INTERPRET_OK;
            case static_cast<uint8_t>(OpCode::OP_CONSTANT):
            {
                const uint8_t index = *ip++; // constant index is on the next instruction
                RuntimeValue value = chunk->constants[index]; // value can maybe be cleared? maybe take by reference and move
                Push(std::move(value));
                break;
            }
            case static_cast<uint8_t>(OpCode::OP_ADD):
            {
                auto right = Pop();
                auto left = Pop();
                
                auto val = std::visit([](auto&& l, auto&& r) -> RuntimeValue {
                    using T1 = std::decay_t<decltype(l)>;
                    using T2 = std::decay_t<decltype(r)>;
                    
                    if constexpr (std::is_same_v<T1, int> && std::is_same_v<T2, int>)
                    {
                        return l + r;
                    }
                    else if constexpr (std::is_same_v<T1, float> && std::is_same_v<T2, float>)
                    {
                        return l + r;
                    }
                    else if constexpr (std::is_same_v<T1, std::string> && std::is_same_v<T2, std::string>)
                    {
                        return l + r;
                    }
                    else if constexpr (std::is_same_v<T1, int> && std::is_same_v<T2, float>)
                    {
                        return static_cast<float>(l) + r;
                    }
                    else if constexpr (std::is_same_v<T1, float> && std::is_same_v<T2, int>)
                    {
                        return l + static_cast<float>(r);
                    }
                    

                    return std::monostate{}; 
                }, left, right);
                
                Push(std::move(val));
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
