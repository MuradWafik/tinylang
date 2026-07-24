#include "vm/VM.h"

#include <stdfloat>
#ifdef DEBUG_TRACE_EXECUTION
#include <print>
#endif

#include "utils/Utils.h"

InterpretResult VM::Interpret(Chunk* chunk)
{
    call_frames.emplace_back(chunk, chunk->code.data(), 0);
    return Run();
}

// #define DEBUG_TRACE_EXECUTION

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
        
        const auto& cur_frame = call_frames.back();
        cur_frame.chunk->DisassembleInstruction(cur_frame.ip - cur_frame.chunk->code.data());
#endif

        switch(static_cast<OpCode>(*(call_frames.back().ip)++))
        {
            case OpCode::OP_RETURN_INT:
            {
                if(const auto terminated = Return<int32_t>())
                {
                    return terminated.value();
                }
                break;
            }
            case OpCode::OP_RETURN_FLOAT:
            {
                if(const auto terminated = Return<std::float32_t>())
                {
                    return terminated.value();
                }
                break;
            }
            case OpCode::OP_RETURN_BOOL:
            {
                if(const auto terminated = Return<bool>())
                {
                    return terminated.value();
                }
                break;
            }
            case OpCode::OP_RETURN_VOID:
            {
                if(const auto terminated = Return())
                {
                    return terminated.value();
                }
                break;
            }
            case OpCode::OP_CONSTANT_INT:
            {
                const auto val = std::get<int32_t>(ExtractNextConstant());
                Push(val);
                break;
            }
            case OpCode::OP_CONSTANT_FLOAT:
            {
                const auto val = std::get<std::float32_t>(ExtractNextConstant());
                Push(val);
                break;
            }
            case OpCode::OP_CONSTANT_BOOL:
            {
                const auto val = std::get<bool>(ExtractNextConstant());
                Push(val);
                break;
            }

            case OpCode::OP_ADD_INT:
            {
                Push(Add<int32_t>());
                break;
            }
            case OpCode::OP_ADD_FLOAT:
            {
                Push(Add<std::float32_t>());
                break;
            }

            case OpCode::OP_SUBTRACT_INT:
            {
                Push(Subtract<int32_t>());
                break;
            }
            case OpCode::OP_SUBTRACT_FLOAT:
            {
                Push(Subtract<std::float32_t>());
                break;
            }

            case OpCode::OP_MULTIPLY_INT:
            {
                Push(Multiply<int32_t>());
                break;
            }
            case OpCode::OP_MULTIPLY_FLOAT:
            {
                Push(Multiply<std::float32_t>());
                break;
            }

            case OpCode::OP_DIVIDE_INT:
            {
                Push(Divide<int32_t>());
                break;
            }
            case OpCode::OP_DIVIDE_FLOAT:
            {
                Push(Divide<std::float32_t>());
                break;
            }

            case OpCode::OP_GREATER_INT:
            {
                Push(GreaterThan<int32_t>());
                break;
            }
            case OpCode::OP_GREATER_FLOAT:
            {
                Push(GreaterThan<std::float32_t>());
                break;
            }

            case OpCode::OP_GREATER_EQUAL_INT:
            {
                Push(GreaterEqualThan<int32_t>());
                break;
            }
            case OpCode::OP_GREATER_EQUAL_FLOAT:
            {
                Push(GreaterEqualThan<std::float32_t>());
                break;
            }

            case OpCode::OP_LESS_INT:
            {
                Push(LessThan<int32_t>());
                break;
            }
            case OpCode::OP_LESS_FLOAT:
            {
                Push(LessThan<std::float32_t>());
                break;
            }

            case OpCode::OP_LESS_EQUAL_INT:
            {
                Push(LessEqualThan<int32_t>());
                break;
            }
            case OpCode::OP_LESS_EQUAL_FLOAT:
            {
                Push(LessEqualThan<std::float32_t>());
                break;
            }

            case OpCode::OP_EQUAL_INT:
            {
                Push(EqualTo<int32_t>());
                break;
            }
            case OpCode::OP_EQUAL_FLOAT:
            {
                Push(EqualTo<std::float32_t>());
                break;
            }
            case OpCode::OP_EQUAL_BOOL:
            {
                Push(EqualTo<bool>());
                break;
            }

            case OpCode::OP_NOT_EQUAL_INT:
            {
                Push(NotEqualTo<int32_t>());
                break;
            }
            case OpCode::OP_NOT_EQUAL_FLOAT:
            {
                Push(NotEqualTo<std::float32_t>());
                break;
            }
            case OpCode::OP_NOT_EQUAL_BOOL:
            {
                Push(NotEqualTo<bool>());
                break;
            }

            // case OpCode::OP_NEGATE:
            // {
            //     Push(std::move(HandleNegate()));
            //     break;
            // }
            case OpCode::OP_DEFINE_GLOBAL_INT:
            {
                DefineGlobal<int32_t>();
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL_FLOAT:
            {
                DefineGlobal<std::float32_t>();
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL_BOOL:
            {
                DefineGlobal<bool>();
                break;
            }

            case OpCode::OP_GET_GLOBAL_INT:
            {
                GetGlobal<int32_t>();
                break;
            }
            case OpCode::OP_GET_GLOBAL_FLOAT:
            {
                GetGlobal<std::float32_t>();
                break;
            }
            case OpCode::OP_GET_GLOBAL_BOOL:
            {
                GetGlobal<bool>();
                break;
            }

            case OpCode::OP_SET_GLOBAL_INT:
            {
                SetGlobal<int32_t>();
                break;
            }
            case OpCode::OP_SET_GLOBAL_FLOAT:
            {
                SetGlobal<std::float32_t>();
                break;
            }
            case OpCode::OP_SET_GLOBAL_BOOL:
            {
                SetGlobal<bool>();
                break;
            }

            case OpCode::OP_CALL:
            {
                CallFunction();
                break;
            }

            case OpCode::OP_GET_LOCAL_INT:
            {
                GetLocalVariable<int32_t>();
                break;
            }
            case OpCode::OP_GET_LOCAL_FLOAT:
            {
                GetLocalVariable<std::float32_t>();
                break;
            }
            case OpCode::OP_GET_LOCAL_BOOL:
            {
                GetLocalVariable<bool>();
                break;
            }

            case OpCode::OP_SET_LOCAL_INT:
            {
                SetLocalVariable<int32_t>();
                break;
            }
            case OpCode::OP_SET_LOCAL_FLOAT:
            {
                SetLocalVariable<std::float32_t>();
                break;
            }
            case OpCode::OP_SET_LOCAL_BOOL:
            {
                SetLocalVariable<bool>();
                break;
            }

            case OpCode::OP_POP_INT:
            {
                Pop<int32_t>();
                break;
            }
            case OpCode::OP_POP_FLOAT:
            {
                Pop<std::float32_t>();
                break;
            }
            case OpCode::OP_POP_BOOL:
            {
                Pop<bool>();
                break;
            }

            case OpCode::OP_JUMP_IF_FALSE:
            {
                HandleJumpIfFalse();
                break;
            }
            case OpCode::OP_JUMP:
            {
                HandleJump();
                break;
            }
            case OpCode::OP_LOOP:
            {
                HandleLoop();
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE_PEEK:
            {
                HandleJumpIfFalsePeek();
                break;
            }
            case OpCode::OP_JUMP_IF_TRUE_PEEK:
            {
                HandleJumpIfTruePeek();
                break;
            }
            case OpCode::OP_LOAD_NATIVE:
            {
                LoadNativeFunction();
                break;
            }
            default: return InterpretResult::INTERPRET_COMPILE_ERROR;
        }
    }
    return InterpretResult::INTERPRET_OK;
}

ConstantValue& VM::ExtractNextConstant()
{
    const uint8_t index = *(call_frames.back().ip)++; // constant index is on the next instruction
    return call_frames.back().chunk->constants[index]; // value can maybe be cleared? maybe take by reference and move
}

void VM::CallFunction()
{
    // cant pop since the function itself and its args must remain in scope
    // for recursion and referencing the args

    const auto arg_bytes = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const size_t func_ptr_index = stack.size() - arg_bytes - sizeof(FunctionObject*);

    if(const auto* function_object = ReadBytesAbsolute<FunctionObject*>(stack, func_ptr_index);
        function_object->is_native())
    {
        const uint8_t* args_ptr = stack.data() + stack.size() - arg_bytes;
        // Buffer for the return value (8 bytes is enough for any primitive)
        uint8_t return_buffer[8];

        function_object->native_fn(args_ptr, return_buffer);

        stack.resize(func_ptr_index);
        // Push the return value if its not void
        if (function_object->return_bytes > 0)
        {
            stack.insert(stack.end(), return_buffer, return_buffer + function_object->return_bytes);
        }
    }
    else
    {
        call_frames.emplace_back(
            function_object->chunk.get(),
            function_object->chunk->code.data(),
            func_ptr_index
        );
    }
}

void VM::HandleJumpIfFalse()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);

    // Pop the true/false condition off the stack
    // TODO: If falsey/bool operators get added handle here?
    if (const auto condition = Pop<bool>(); condition == false)
    {
        call_frames.back().ip += offset;
    }
}

void VM::HandleJump()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    call_frames.back().ip += offset;
}

void VM::HandleLoop()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    call_frames.back().ip -= offset;
}

void VM::HandleJumpIfFalsePeek()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);

    // TODO: If falsey/bool operators get added handle here?
    if(const auto condition = Peek<bool>(); condition == false)
    {
        call_frames.back().ip += offset;
    }
}

void VM::HandleJumpIfTruePeek()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);

    if(const auto condition = Peek<bool>(); condition == true)
    {
        call_frames.back().ip += offset;
    }
}

void VM::LoadNativeFunction()
{
    const std::string& lib_path = std::get<std::string>(ExtractNextConstant());
    const std::string& symbol_name = std::get<std::string>(ExtractNextConstant());
    const auto num_args = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto return_bytes = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    auto result = plugin_loader.LoadSymbol(lib_path, symbol_name);
    if (!result)
    {
        throw std::runtime_error(result.error());
    }

    globals[symbol_name] = std::make_shared<FunctionObject>(symbol_name, num_args, return_bytes, result.value());
}
