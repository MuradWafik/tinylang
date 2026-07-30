#include "vm/VM.h"

#include <stdfloat>
#ifdef DEBUG_TRACE_EXECUTION
#include <print>
#endif

#include "utils/Utils.h"

InterpretResult VM::StartProgram(Chunk* root_chunk)
{
    // Run the root chunk to evaluate global functions and variables
    if(const auto res = Interpret(root_chunk); res != InterpretResult::INTERPRET_OK) return res;

    if(!globals.contains("main"))
    {
        std::println(std::cerr, "Runtime Error: No 'main' entrypoint found.");
        return InterpretResult::INTERPRET_RUNTIME_ERROR;
    }

    const auto* main_func_ptr_ptr = std::get_if<FunctionObject*>(&globals["main"]);
    if(!main_func_ptr_ptr)
    {
        std::println(std::cerr, "Runtime Error: 'main' is not a function.");
        return InterpretResult::INTERPRET_RUNTIME_ERROR;
    }
    const auto main_func_ptr = *main_func_ptr_ptr;
    
    // Push main so it acts as the first call frame
    Push<FunctionObject*>(main_func_ptr);

    CallFrame main_frame;
    main_frame.chunk = main_func_ptr->chunk.get();
    main_frame.ip = main_frame.chunk->code.data();
    main_frame.stack_base = stack.size() - sizeof(FunctionObject*); // stack starts here
    call_frames.push_back(main_frame);

    return Run();
}

InterpretResult VM::Interpret(Chunk* chunk)
{
    // when in function local scope adds the offset but this needs to be done before main runs
    Push<Object*>(nullptr);
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
            case OpCode::OP_RETURN_OBJECT:
            {
                if(const auto terminated = Return<Object*>())
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
            case OpCode::OP_CONSTANT_FUNCTION:
            {
                const auto val = std::get<FunctionObject*>(ExtractNextConstant());
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
            case OpCode::OP_ADD_STRING:
            {
                AddString();
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
            case OpCode::OP_NEGATE_INT:
            {
                Push(-Pop<int32_t>());
                break;
            }
            case OpCode::OP_NEGATE_FLOAT:
            {
                Push(-Pop<std::float32_t>());
                break;
            }
            case OpCode::OP_MOD_INT:
            {
                auto right = Pop<int32_t>();
                auto left = Pop<int32_t>();
                Push(left % right);
                break;
            }
            case OpCode::OP_NOT_BOOL:
            {
                Push(!Pop<bool>());
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
            case OpCode::OP_DEFINE_GLOBAL_FUNCTION:
            {
                DefineGlobal<FunctionObject*>();
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL_OBJECT:
            {
                DefineGlobal<Object*>();
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
            case OpCode::OP_GET_GLOBAL_FUNCTION:
            {
                GetGlobal<FunctionObject*>();
                break;
            }
            case OpCode::OP_GET_GLOBAL_OBJECT:
            {
                GetGlobal<Object*>();
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
            case OpCode::OP_SET_GLOBAL_OBJECT:
            {
                SetGlobal<Object*>();
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
            case OpCode::OP_GET_LOCAL_OBJECT:
            {
                GetLocalVariable<Object*>();
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
            case OpCode::OP_SET_LOCAL_OBJECT:
            {
                SetLocalVariable<Object*>();
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
            case OpCode::OP_POP_OBJECT:
            {
                Pop<Object*>();
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
            case OpCode::OP_ALLOCATE_STRING:
            {
                AllocateString();
                break;
            }
            case OpCode::OP_ALLOCATE_ARRAY:
            {
                AllocateArray();
                break;
            }
            case OpCode::OP_GET_INDEX:
            {
                GetArrayIndex();
                break;
            }
            case OpCode::OP_SET_INDEX:
            {
                SetArrayIndex();
                break;
            }
            case OpCode::OP_ALLOCATE_STRUCT:
            {
                AllocateStruct();
                break;
            }
            case OpCode::OP_GET_PROPERTY:
            {
                GetProperty();
                break;
            }
            case OpCode::OP_GET_LENGTH:
            {
                GetLength();
                break;
            }
            case OpCode::OP_SET_PROPERTY:
            {
                SetProperty();
                break;
            }

            default: return InterpretResult::INTERPRET_COMPILE_ERROR;
        }
    }
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
        if(function_object->return_bytes > 0)
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
    if(const auto condition = Pop<bool>(); condition == false)
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
    if(!result)
    {
        throw std::runtime_error(result.error());
    }

    allocated_native_functions.push_back(std::make_unique<FunctionObject>(symbol_name, num_args, return_bytes, result.value()));
    globals[symbol_name] = allocated_native_functions.back().get();
}

void VM::AllocateArray()
{
    //OP_ALLOCATE_ARRAY [2 bytes: element_count] [1 byte: stride]
    const auto element_count = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    const size_t total_bytes = element_count * bytes_per_element;
    const uint8_t* elements_ptr = stack.data() + stack.size() - total_bytes;
    auto* arr = AllocateObject<Array>(elements_ptr, element_count, bytes_per_element);

    stack.resize(stack.size() - total_bytes);

    Push<Object*>(arr);
   /// Stack: Pops (count*stride) bytes, allocates ArrayObject, pushes 8 byte Object*
}

void VM::GetArrayIndex()
{
    // OP_GET_INDEX [1 byte: stride]    | Stack: Pops 4 byte int index, pops 8 byte ArrayObject*, pushes 'stride' bytes
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto index = Pop<int32_t>();
    const auto* array = dynamic_cast<Array*>(Pop<Object*>());

    if(index < 0 || index >= array->size)
    {
        throw std::runtime_error(std::format("Array index out of bounds. Index: {}, Size: {}", index, array->size));
    }

    const size_t byte_offset = index * bytes_per_element;
    stack.insert(stack.end(), 
                 &array->elements[byte_offset], 
                 &array->elements[byte_offset + bytes_per_element]);
}

void VM::SetArrayIndex()
{
    // OP_SET_INDEX [1 byte: stride]  | Stack: [ArrayObject*] [4 byte int index] ['stride' bytes value]
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    // have to remove the array and index from the stack, but leave the value bytes
    // So need to read everything using absolute indexing instead of popping
    const size_t value_start = stack.size() - bytes_per_element;
    const size_t index_start = value_start - sizeof(int32_t);
    const size_t array_start = index_start - sizeof(Object*);
    const auto index = ReadBytesAbsolute<int32_t>(stack, index_start);
    const auto* array = dynamic_cast<Array*>(ReadBytesAbsolute<Object*>(stack, array_start));

    if(index < 0 || index >= array->size)
    {
        throw std::runtime_error(std::format("Array index out of bounds. Index: {}, Size: {}", index, array->size));
    }

    const size_t byte_offset = index * bytes_per_element;

    // Copy the full stride of value bytes directly into the array buffer
    std::memcpy(&array->elements[byte_offset], &stack[value_start], bytes_per_element);

    // shift the value bytes down to overwrite the array and index
    std::memmove(&stack[array_start], &stack[value_start], bytes_per_element);
    stack.resize(stack.size() - sizeof(Object*) - sizeof(int32_t));
}

void VM::AllocateStruct()
{
    // OP_ALLOCATE_STRUCT, [2 bytes: total_size] [1 byte: from_stack]
    const auto heap_size = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    if(const auto from_stack = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip))
    {
        const uint8_t* struct_fields_ptr = stack.data() + stack.size() - heap_size;
        auto* obj = AllocateObject<Struct>(struct_fields_ptr, heap_size);
        stack.resize(stack.size() - heap_size);
        Push<Object*>(obj);
    }
    else
    {
        auto* obj = AllocateObject<Struct>(nullptr, heap_size);
        Push<Object*>(obj);
    }
}

void VM::GetLength()
{
    auto* obj = Pop<Object*>();
    if(const auto* arr = dynamic_cast<Array*>(obj))
    {
        Push<int32_t>(static_cast<int32_t>(arr->size));
    }
    else if(const auto* str = dynamic_cast<String*>(obj))
    {
        Push<int32_t>(static_cast<int32_t>(str->length));
    }
    else
    {
        throw std::runtime_error("Attempted to get length of object that is not an array or string");
    }
}

void VM::GetProperty()
{
    // OP_GET_PROPERTY [2 bytes: byte_offset] [1 byte: size] | Stack: Pops 8 byte StructObject*, pushes 'size' bytes from offset
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto* obj = dynamic_cast<Struct*>(Pop<Object*>());

    stack.insert(
        stack.end(),
        &obj->fields[offset],
        &obj->fields[offset + size]
    );
}

void VM::SetProperty()
{
    // OP_SET_PROPERTY [2 bytes: byte_offset] [1 byte: size] | Stack: Pops 'size' bytes, pops 8 byte StructObject*, writes bytes, pushes bytes back
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    const size_t value_start = stack.size() - size;
    const size_t struct_start = value_start - sizeof(Object*);
    auto* obj = dynamic_cast<Struct*>(ReadBytesAbsolute<Object*>(stack, struct_start));

    // Copies values from the stack to the struct
    std::memcpy(&obj->fields[offset], &stack[value_start], size);

    // shift the value bytes down to overwrite the struct pointer
    std::memmove(&stack[struct_start], &stack[value_start], size);
    stack.resize(stack.size() - sizeof(Object*));
}

void VM::AllocateString()
{
    const auto index = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto str = std::get<std::string>(call_frames.back().chunk->constants[index]);
    auto* obj = AllocateObject<String>(str.c_str(), str.length());
    Push<Object*>(obj);
}

void VM::AddString()
{
    const auto r = dynamic_cast<String*>(Pop<Object*>());
    const auto l = dynamic_cast<String*>(Pop<Object*>());

    auto* obj = AllocateObject<String>(l, r);
    Push<Object*>(obj);
}
