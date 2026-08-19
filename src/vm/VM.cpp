#include <iostream>
#include <cstring>
#include "vm/VM.h"

#include <stdfloat>
#ifdef DEBUG_TRACE_EXECUTION
#include <print>
#endif

#include "utils/Utils.h"

InterpretResult VM::StartProgram(const std::unordered_map<std::string, std::unique_ptr<Chunk>>& chunks, const std::vector<std::string>& ordered_modules)
{
    for(const auto& name : ordered_modules)
    {
        if(const auto it = chunks.find(name); it != chunks.end())
        {
            if(const auto res = Interpret(it->second.get()); res != InterpretResult::INTERPRET_OK)
            {
                return res;
            }
        }
    }

    return InterpretResult::INTERPRET_OK;
}

InterpretResult VM::Interpret(Chunk* chunk)
{
    // when in function local scope adds the offset but this needs to be done before main runs
    Push<Object*>(nullptr);
    call_frames.emplace_back(chunk, chunk->code.data(), 0);
    try
    {
        return Run();
    }
    catch(const std::exception& e)
    {
        size_t line = 0;
        if(!call_frames.empty())
        {
            const auto& cur_frame = call_frames.back();
            const auto offset = static_cast<size_t>(cur_frame.ip - cur_frame.chunk->code.data());
            if(offset < cur_frame.chunk->lines.size())
            {
                line = cur_frame.chunk->lines[offset];
            }
        }

        if(line > 0)
        {
            std::println(std::cerr, "Runtime Error: {} at line {}", e.what(), line);
        }
        else
        {
            std::println(std::cerr, "Runtime Error: {}", e.what());
        }
        return InterpretResult::INTERPRET_RUNTIME_ERROR;
    }
}

// #define DEBUG_TRACE_EXECUTION

InterpretResult VM::Run()
{
    while(true)
    {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        for (const auto& slot : stack) {
            std::cout << "[ " << static_cast<int>(slot) << " ]";
        }
        std::cout << std::endl;
        
        const auto& cur_frame = call_frames.back();
        cur_frame.chunk->DisassembleInstruction(cur_frame.ip - cur_frame.chunk->code.data());
#endif

        switch(static_cast<OpCode>(*(call_frames.back().ip)++))
        {
            case OpCode::OP_RETURN:
            {
                const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
                if(const auto terminated = Return(size))
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
            case OpCode::OP_CONSTANT_CHAR:
            {
                const auto val = std::get<char8_t>(ExtractNextConstant());
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
                const auto right = Pop<int32_t>();
                const auto left = Pop<int32_t>();
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
            case OpCode::OP_GREATER_CHAR:
            {
                Push(GreaterThan<char8_t>());
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
            case OpCode::OP_GREATER_EQUAL_CHAR:
            {
                Push(GreaterEqualThan<char8_t>());
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
            case OpCode::OP_LESS_CHAR:
            {
                Push(LessThan<char8_t>());
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
            case OpCode::OP_LESS_EQUAL_CHAR:
            {
                Push(LessEqualThan<char8_t>());
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
            case OpCode::OP_EQUAL_CHAR:
            {
                Push(EqualTo<char8_t>());
                break;
            }
            case OpCode::OP_EQUAL_STRING:
            {
                EqualString();
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
            case OpCode::OP_NOT_EQUAL_CHAR:
            {
                Push(NotEqualTo<char8_t>());
                break;
            }
            case OpCode::OP_NOT_EQUAL_STRING:
            {
                NotEqualString();
                break;
            }
            case OpCode::OP_DEFINE_GLOBAL:
            {
                DefineGlobal();
                break;
            }
            case OpCode::OP_GET_GLOBAL:
            {
                const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
                const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

                if(offset + size > globals.size())
                {
                    std::println(std::cerr, "Runtime Error: Global offset out of bounds.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                stack.insert(stack.end(), globals.begin() + offset, globals.begin() + offset + size);
                break;
            }
            case OpCode::OP_SET_GLOBAL:
            {
                const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
                const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

                if(offset + size > globals.size())
                {
                    std::println(std::cerr, "Runtime Error: Global offset out of bounds.");
                    return InterpretResult::INTERPRET_RUNTIME_ERROR;
                }

                // read from stack, without popping
                for(int i = 0; i < size; i++)
                {
                    globals[offset + i] = stack[stack.size() - size + i];
                }
                break;
            }

            case OpCode::OP_CALL:
            {
                CallFunction();
                break;
            }

            case OpCode::OP_GET_LOCAL:
            {
                GetLocal();
                break;
            }
            case OpCode::OP_SET_LOCAL:
            {
                SetLocal();
                break;
            }
            case OpCode::OP_POP:
            {
                Pop();
                break;
            }
            case OpCode::OP_DUP:
            {
                Duplicate();
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
            case OpCode::OP_ALLOCATE_ARRAY_DEFAULT:
            {
                AllocateArrayDefault();
                break;
            }
            case OpCode::OP_ARRAY_TO_STRING:
            {
                ArrayToString();
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
            case OpCode::OP_GET_STRING_CHAR:
            {
                GetStringChar();
                break;
            }
            case OpCode::OP_BOX_ANY:
            {
                BoxAny();
                break;
            }
            case OpCode::OP_CAST_CHECK:
            {
                CastCheck();
                break;
            }
            case OpCode::OP_IS_CHECK:
            {
                IsCheck();
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
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto num_args = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto return_bytes = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    std::string path_to_load = lib_path;
    if(!std::filesystem::exists(path_to_load) && !project_root.empty())
    {
        if(const auto candidate = project_root / lib_path; std::filesystem::exists(candidate))
        {
            path_to_load = candidate.string();
        }
    }

    auto result = plugin_loader.LoadSymbol(path_to_load, symbol_name);
    if(!result)
    {
        throw std::runtime_error(result.error());
    }

    allocated_native_functions.push_back(std::make_unique<FunctionObject>(symbol_name, num_args, return_bytes, result.value()));
    
    if(offset + sizeof(FunctionObject*) > globals.size())
    {
        globals.resize(offset + sizeof(FunctionObject*));
    }
    
    FunctionObject* ptr = allocated_native_functions.back().get();
    std::memcpy(globals.data() + offset, &ptr, sizeof(FunctionObject*));
}

void VM::AllocateArray()
{
    //OP_ALLOCATE_ARRAY [2 bytes: element_count] [1 byte: stride] [1 byte: element_kind]
    const auto element_count = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto element_kind = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    const size_t total_bytes = element_count * bytes_per_element;
    const uint8_t* elements_ptr = stack.data() + stack.size() - total_bytes;
    auto* arr = AllocateObject<Array>(elements_ptr, element_count, bytes_per_element, element_kind);

    stack.resize(stack.size() - total_bytes);

    Push<Object*>(arr);
}

void VM::AllocateArrayDefault()
{
    // OP_ALLOCATE_ARRAY_DEFAULT [1 byte: stride] [1 byte: element_kind]
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto element_kind = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto count = Pop<int32_t>();

    if(count < 0)
    {
        throw std::runtime_error(std::format("Array size cannot be negative: {}", count));
    }

    auto* arr = AllocateObject<Array>(nullptr, static_cast<size_t>(count), bytes_per_element, element_kind);

    if(static_cast<ArrayElementKind>(element_kind) == ArrayElementKind::String && count > 0)
    {
        for(size_t i = 0; i < static_cast<size_t>(count); ++i)
        {
            auto* empty_str = AllocateObject<String>("", 0);
            std::memcpy(arr->elements + i * sizeof(String*), &empty_str, sizeof(String*));
        }
    }

    Push<Object*>(arr);
}

std::string VM::FormatArray(const Array* array)
{
    std::string result = "[";
    if(array && array->elements && array->size > 0)
    {
        for(size_t i = 0; i < array->size; ++i)
        {
            if(i > 0)
            {
                result += ", ";
            }

            const size_t offset = i * array->bytes_per_element;
            switch(static_cast<ArrayElementKind>(array->element_kind))
            {
                case ArrayElementKind::Int:
                {
                    int32_t val = 0;
                    std::memcpy(&val, &array->elements[offset], sizeof(int32_t));
                    result += std::to_string(val);
                    break;
                }
                case ArrayElementKind::Float:
                {
                    std::float32_t val = 0.0f;
                    std::memcpy(&val, &array->elements[offset], sizeof(std::float32_t));
                    result += std::format("{}", val);
                    break;
                }
                case ArrayElementKind::Bool:
                {
                    bool val = false;
                    std::memcpy(&val, &array->elements[offset], sizeof(bool));
                    result += (val ? "true" : "false");
                    break;
                }
                case ArrayElementKind::Char:
                {
                    char8_t val = 0;
                    std::memcpy(&val, &array->elements[offset], sizeof(char8_t));
                    result += '\'';
                    result += static_cast<char>(val);
                    result += '\'';
                    break;
                }
                case ArrayElementKind::String:
                {
                    String* val = nullptr;
                    std::memcpy(&val, &array->elements[offset], sizeof(String*));
                    result += '"';
                    if(val && val->chars)
                    {
                        result.append(val->chars, val->length);
                    }
                    result += '"';
                    break;
                }
                case ArrayElementKind::Array:
                {
                    Array* val = nullptr;
                    std::memcpy(&val, &array->elements[offset], sizeof(Array*));
                    result += FormatArray(val);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
    result += "]";
    return result;
}

void VM::ArrayToString()
{
    const auto* array = static_cast<Array*>(Pop<Object*>());
    const std::string result = FormatArray(array);
    auto* str_obj = AllocateObject<String>(result.c_str(), result.length());
    Push<Object*>(str_obj);
}

void VM::GetArrayIndex()
{
    // OP_GET_INDEX [1 byte: stride]    | Stack: Pops 4 byte int index, pops 8 byte ArrayObject*, pushes 'stride' bytes
    const auto bytes_per_element = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const auto index = Pop<int32_t>();
    const auto* array = static_cast<Array*>(Pop<Object*>());

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
    const auto* array = static_cast<Array*>(ReadBytesAbsolute<Object*>(stack, array_start));

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

void VM::GetStringChar()
{
    const auto index = Pop<int32_t>();
    const auto* str = static_cast<String*>(Pop<Object*>());

    if(index < 0 || index >= str->length)
    {
        throw std::runtime_error(std::format("String index out of bounds. Index: {}, Size: {}", index, str->length));
    }

    Push<char8_t>(str->chars[index]);
}

void VM::AllocateStruct()
{
    // OP_ALLOCATE_STRUCT, [2 bytes: total_size] [1 byte: from_stack]
    const auto heap_size = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    if(ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip))
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
    if(auto* obj = Pop<Object*>(); obj->type == ObjectType::Array)
    {
        Push<int32_t>(static_cast<int32_t>(static_cast<Array*>(obj)->size));
    }
    else if(obj->type == ObjectType::String)
    {
        Push<int32_t>(static_cast<int32_t>(static_cast<String*>(obj)->length));
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
    const auto* obj = static_cast<Struct*>(Pop<Object*>());

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
    const auto* obj = static_cast<Struct*>(ReadBytesAbsolute<Object*>(stack, struct_start));

    // Copy values from stack to the struct
    std::memcpy(&obj->fields[offset], &stack[value_start], size);

    // Shift the value bytes down to overwrite the struct pointer
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
    const auto r = static_cast<String*>(ReadBytesAbsolute<Object*>(stack, stack.size() - sizeof(Object*)));
    const auto l = static_cast<String*>(ReadBytesAbsolute<Object*>(stack, stack.size() - 2 * sizeof(Object*)));

    auto* obj = AllocateObject<String>(l, r);
    stack.resize(stack.size() - 2 * sizeof(Object*));
    Push<Object*>(obj);
}

void VM::EqualString()
{
    const auto r = static_cast<String*>(Pop<Object*>());
    const auto l = static_cast<String*>(Pop<Object*>());
    
    if(l->length != r->length)
    {
        Push<bool>(false);
        return;
    }
    Push<bool>(std::memcmp(l->chars, r->chars, l->length) == 0);
}

void VM::NotEqualString()
{
    const auto r = static_cast<String*>(Pop<Object*>());
    const auto l = static_cast<String*>(Pop<Object*>());
    
    if(l->length != r->length)
    {
        Push<bool>(true);
        return;
    }
    Push<bool>(std::memcmp(l->chars, r->chars, l->length) != 0);
}

void VM::GetLocal()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    const auto base = call_frames.back().stack_base + sizeof(FunctionObject*);
    stack.insert(stack.end(), stack.begin() + base + offset, stack.begin() + base + offset + size);
}

void VM::DefineGlobal()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    // Ensure globals has enough space
    if(offset + size > globals.size())
    {
        globals.resize(offset + size);
    }

    for(int i = size - 1; i >= 0; i--)
    {
        globals[offset + i] = stack.back();
        stack.pop_back();
    }
}

void VM::SetLocal()
{
    const auto offset = ReadAndAdvanceBytes<uint16_t>(call_frames.back().ip);
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    const auto base = call_frames.back().stack_base + sizeof(FunctionObject*);
    for(int i = 0; i < size; i++)
    {
        stack[base + offset + i] = stack[stack.size() - size + i];
    }
}
void VM::Pop()
{
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    for(int i = 0; i < size; i++)
    {
        stack.pop_back();
    }
}

void VM::Duplicate()
{
    const auto size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);
    const size_t start_idx = stack.size() - size;
    for(int i = 0; i < size; i++)
    {
        uint8_t val = stack[start_idx + i];
        stack.push_back(val);
    }
}


void VM::BoxAny()
{
    const auto source_type_id = ReadAndAdvanceBytes<uint32_t>(call_frames.back().ip);
    const auto value_size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    std::vector<uint8_t> value_bytes(stack.end() - value_size, stack.end());

    stack.resize(stack.size() - value_size);

    // Bytes 0-3  : source_type_id (4 bytes)
    // Bytes 4-7  : padding (4 bytes of zeroes)
    // Bytes 8-15 : payload (8 bytes: value_bytes + zero padding if < 8 bytes)
    WriteBytes(stack, source_type_id);
    WriteBytes(stack, static_cast<uint32_t>(0)); // padding

    stack.insert(stack.end(), value_bytes.begin(), value_bytes.end());
    if(value_bytes.size() < 8)
    {
        stack.insert(stack.end(), 8 - value_bytes.size(), 0); // pad payload to 8 bytes
    }
}


void VM::CastCheck()
{
    const auto target_type_id = ReadAndAdvanceBytes<uint32_t>(call_frames.back().ip);
    const auto target_size = ReadAndAdvanceBytes<uint8_t>(call_frames.back().ip);

    // The top 16 bytes of stack is the any
    const size_t any_offset = stack.size() - 16;

    uint32_t actual_type_id;
    std::memcpy(&actual_type_id, stack.data() + any_offset, sizeof(uint32_t));

    // Ensure runtime typeid matches target
    if(actual_type_id != target_type_id)
    {
        throw std::runtime_error(std::format(
            "Runtime Cast Error: Invalid cast from type ID {} to type ID {}", actual_type_id, target_type_id)
        );
    }

    std::vector<uint8_t> payload_bytes(
        stack.data() + any_offset + 8,
        stack.data() + any_offset + 8 + target_size
    );

    stack.resize(stack.size() - 16);
    stack.insert(stack.end(), payload_bytes.begin(), payload_bytes.end());
}

void VM::IsCheck()
{
    const auto target_type_id = ReadAndAdvanceBytes<uint32_t>(call_frames.back().ip);
    const size_t any_offset = stack.size() - 16;

    uint32_t actual_type_id;
    std::memcpy(&actual_type_id, stack.data() + any_offset, sizeof(uint32_t));

    const bool is_match = (actual_type_id == target_type_id);
    stack.resize(stack.size() - 16);
    Push(is_match);
}