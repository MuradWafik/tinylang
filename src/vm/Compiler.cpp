#include "vm/Compiler.h"
#include "analysis/Type.h"
#include "project/ModuleRegistry.h"
#include "project/ProjectConfig.h"
#include "utils/Constants.h"

#include <algorithm>
#include <variant>

std::unordered_map<std::string, std::unique_ptr<Chunk>> Compiler::CompileAll(const std::string& entry_module)
{
    std::unordered_map<std::string, std::unique_ptr<Chunk>> chunks;
    
    // Assign offsets to all global variables and functions for the initial pass
    for(const auto& namespace_obj: registry->GetNamespaces() | std::views::values)
    {
        for(const auto& ast_node : namespace_obj->asts)
        {
            auto assign_global = [&](const std::string& lexeme, const size_t size)
            {
                if(!global_offsets.contains(lexeme))
                {
                    global_offsets[lexeme] = global_bytes_count;
                    global_bytes_count += size;
                }
            };

            if(const auto* var_decl = dynamic_cast<const VariableDeclaration*>(ast_node.get()))
            {
                assign_global(var_decl->name.lexeme, var_decl->type_info->GetSize());
            }
            else if(const auto* func_decl = dynamic_cast<const FunctionDeclaration*>(ast_node.get()))
            {
                assign_global(func_decl->method_signature.name.lexeme, 8);
            }
            else if (const auto* native_func = dynamic_cast<const NativeFunctionDeclaration*>(ast_node.get()))
            {
                assign_global(native_func->method_signature.name.lexeme, 8);
            }
            else if (const auto* native_mod = dynamic_cast<const NativeImportStatement*>(ast_node.get()))
            {
                assign_global(native_mod->name.lexeme, 8);
            }
        }
    }

    for(const auto& [name, namespace_obj] : registry->GetNamespaces())
    {
        current_chunk = std::make_unique<Chunk>();
        
        for(const auto& ast_node : namespace_obj->asts)
        {
            if (dynamic_cast<ModuleDeclaration*>(ast_node.get()) || dynamic_cast<ImportStatement*>(ast_node.get())) continue;
            CompileStatement(dynamic_cast<Statement*>(ast_node.get()));
        }
        
        const auto last_line = current_chunk->lines.empty() ? 0 : current_chunk->lines.back();
        
        if (name == entry_module && global_offsets.contains("main"))
        {
            current_chunk->has_main = true;
            const uint16_t offset = global_offsets.at("main");
            current_chunk->WriteInstruction(last_line, OpCode::OP_GET_GLOBAL, offset, static_cast<uint8_t>(sizeof(FunctionObject*))); // main is a function object
            current_chunk->WriteInstruction(last_line, OpCode::OP_CALL, static_cast<uint16_t>(0)); // 0 bytes of args
        }
        
        current_chunk->WriteInstruction(last_line, OpCode::OP_RETURN, static_cast<uint8_t>(0));
        
        chunks[name] = std::move(current_chunk);
    }
    
    return chunks;
}

void Compiler::CompileStatement(const Statement* statement)
{
    if(const auto var_decl = dynamic_cast<const VariableDeclaration*>(statement)) return CompileVariableDeclaration(var_decl);
    if(const auto func_decl = dynamic_cast<const FunctionDeclaration*>(statement)) return CompileFunctionDeclaration(func_decl);
    if(const auto ret_stmt = dynamic_cast<const ReturnStatement*>(statement)) return CompileReturnStatement(ret_stmt);
    if(const auto body_stmt = dynamic_cast<const BodyStatement*>(statement)) return CompileBodyStatement(body_stmt);
    if(const auto if_stmt = dynamic_cast<const IfStatement*>(statement)) return CompileIfStatement(if_stmt);
    if(const auto while_stmt = dynamic_cast<const WhileStatement*>(statement)) return CompileWhileStatement(while_stmt);
    if(const auto expr_stmt = dynamic_cast<const ExpressionStatement*>(statement)) return CompileExpressionStatement(expr_stmt);
    if(const auto cnt_stmt = dynamic_cast<const ContinueStatement*>(statement)) return CompileContinueStatement(cnt_stmt);
    if(const auto brk_stmt = dynamic_cast<const BreakStatement*>(statement)) return CompileBreakStatement(brk_stmt);
    if(const auto mod_stmt = dynamic_cast<const NativeImportStatement*>(statement)) return CompileNativeModuleStatement(mod_stmt);
    if(const auto native_fn_decl = dynamic_cast<const NativeFunctionDeclaration*>(statement)) return CompileNativeFunctionDeclaration(native_fn_decl);
    if(const auto for_loop = dynamic_cast<const ForLoop*>(statement)) return CompileForLoop(for_loop);
}

void Compiler::CompileExpression(const Expression* expression)
{
    const auto line = expression->source_location.line_number;
    if(const auto float_lit = dynamic_cast<const FloatLiteral*>(expression)) return CompileLiteral(float_lit->value, line);
    if(const auto int_lit = dynamic_cast<const IntegerLiteral*>(expression)) return CompileLiteral(int_lit->value, line);
    if(const auto bool_lit = dynamic_cast<const BoolLiteral*>(expression)) return CompileLiteral(bool_lit->value, line);
    if(const auto char_lit = dynamic_cast<const CharLiteral*>(expression)) return CompileLiteral(char_lit->value, line);
    if(const auto string_lit = dynamic_cast<const StringLiteral*>(expression)) return CompileLiteral(string_lit->value, line);
    if(const auto binary_expr = dynamic_cast<const BinaryExpression*>(expression)) return CompileBinaryExpression(binary_expr);
    if(const auto unary_expr = dynamic_cast<const UnaryExpression*>(expression)) return CompileUnaryExpression(unary_expr);
    if(const auto iden_expr = dynamic_cast<const IdentifierExpression*>(expression)) return CompileIdentifierExpression(iden_expr);
    if(const auto call_expr = dynamic_cast<const CallExpression*>(expression)) return CompileCallExpression(call_expr);
    if(const auto asgn_expr = dynamic_cast<const AssignmentExpression*>(expression)) return CompileAssignmentExpression(asgn_expr);
    if(const auto array_lit = dynamic_cast<const ArrayLiteral*>(expression)) return CompileArrayLiteral(array_lit);
    if(const auto index_access = dynamic_cast<const IndexAccess*>(expression)) return CompileIndexAccess(index_access);
    if(const auto property_access = dynamic_cast<const PropertyAccess*>(expression)) return CompilePropertyAccess(property_access);
    if(const auto switch_expr = dynamic_cast<const SwitchExpression*>(expression)) return CompileSwitchExpression(switch_expr);
    if(const auto cast_expr = dynamic_cast<const CastExpression*>(expression)) return CompileCastExpression(cast_expr);
    if(const auto is_expr = dynamic_cast<const IsExpression*>(expression)) return CompileIsExpression(is_expr);
}

void Compiler::CompileLiteral(const ConstantValue& value, const uint32_t line) const
{
    const auto index = current_chunk->AddConstant(value);
    assert(index < 256 && "Too many constants in current scope, overflowing");
    
    if(std::holds_alternative<int32_t>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, static_cast<uint8_t>(index));
    else if(std::holds_alternative<std::float32_t>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FLOAT, static_cast<uint8_t>(index));
    else if(std::holds_alternative<bool>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_BOOL, static_cast<uint8_t>(index));
    else if(std::holds_alternative<char8_t>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_CHAR, static_cast<uint8_t>(index));
    else if(std::holds_alternative<std::string>(value)) current_chunk->WriteInstruction(line, OpCode::OP_ALLOCATE_STRING, static_cast<uint8_t>(index));
}

void Compiler::CompileLogicalAnd(const BinaryExpression* binary_expression)
{
    CompileExpression(binary_expression->left.get());

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_JUMP_IF_FALSE_PEEK, static_cast<uint16_t>(0));
    const size_t jump_index = current_chunk->code.size() - 2;

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP, static_cast<uint8_t>(1));

    CompileExpression(binary_expression->right.get());

    const uint16_t distance = current_chunk->code.size() - (jump_index + 2);
    current_chunk->code[jump_index] = (distance >> 8) & 0xff; // High byte
    current_chunk->code[jump_index + 1] = distance & 0xff; // Low byte
}

void Compiler::CompileLogicalOr(const BinaryExpression* binary_expression)
{
    CompileExpression(binary_expression->left.get());

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_JUMP_IF_TRUE_PEEK, static_cast<uint16_t>(0));
    const size_t jump_index = current_chunk->code.size() - 2;

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP, static_cast<uint8_t>(1));

    CompileExpression(binary_expression->right.get());

    const uint16_t distance = current_chunk->code.size() - (jump_index + 2);
    current_chunk->code[jump_index] = (distance >> 8) & 0xff; // High byte
    current_chunk->code[jump_index + 1] = distance & 0xff; // Low byte
}

void Compiler::CompileBinaryExpression(const BinaryExpression* binary_expression)
{
    const auto operator_token = binary_expression->operator_token.type;
    // have their own optimization of short-circuiting so cant precompile left and right sides
    if(operator_token == TokenType::AndAnd)
    {
        return CompileLogicalAnd(binary_expression);
    }
    if(operator_token == TokenType::OrOr)
    {
        return CompileLogicalOr(binary_expression);
    }

    CompileExpression(binary_expression->left.get());
    CompileExpression(binary_expression->right.get());

    const auto line = binary_expression->source_location.line_number;

    // now has to manually dictate which opcode to write based on the type since primitives are treated as is
    const Type* type = binary_expression->left->type_info;

    switch(binary_expression->operator_token.type)
    {
        case TokenType::Plus:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_FLOAT);
            if(type == PrimitiveType::String.get()) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_STRING);
            break;
        }
        case TokenType::Minus:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_SUBTRACT_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_SUBTRACT_FLOAT);
            break;
        }
        case TokenType::Star:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_MULTIPLY_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_MULTIPLY_FLOAT);
            break;
        }
        case TokenType::Slash:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_DIVIDE_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_DIVIDE_FLOAT);
            break;
        }
        case TokenType::Modulo:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_MOD_INT);
            break;
        }
        case TokenType::Greater:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_FLOAT);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_CHAR);
            break;
        }
        case TokenType::GreaterEqual:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_EQUAL_FLOAT);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_EQUAL_CHAR);
            break;
        }
        case TokenType::Less:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_FLOAT);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_CHAR);
            break;
        }
        case TokenType::LessEqual:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_EQUAL_FLOAT);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_EQUAL_CHAR);
            break;
        }
        case TokenType::Equal:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_FLOAT);
            if(type == PrimitiveType::Bool.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_BOOL);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_CHAR);
            if(type == PrimitiveType::String.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_STRING);
            break;
        }
        case TokenType::NotEqual:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_FLOAT);
            if(type == PrimitiveType::Bool.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_BOOL);
            if(type == PrimitiveType::Char.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_CHAR);
            if(type == PrimitiveType::String.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_STRING);
            break;
        }
        default: assert(false && "Unexpectedly reached default case compiling binary expression");
    }
}

void Compiler::CompileUnaryExpression(const UnaryExpression* unary_expression)
{
    CompileExpression(unary_expression->right.get());

    const auto line = unary_expression->source_location.line_number;
    const Type* type = unary_expression->right->type_info;

    switch(unary_expression->operator_token.type)
    {
        case TokenType::Minus:
        {
            if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) return current_chunk->WriteInstruction(line, OpCode::OP_NEGATE_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NEGATE_FLOAT);
            break;
        }
        case TokenType::Negate:
        {
            if(type == PrimitiveType::Bool.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_BOOL);
            break;
        }
        default: assert(false && "Unexpectedly reached default case compiling unary expression");
    }
}

void Compiler::CompileVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    const Type* type = variable_declaration->type_info;
    
    if(variable_declaration->initializer)
    {
        CompileExpression(variable_declaration->initializer.get());
    }
    else
    {
        const auto line = variable_declaration->source_location.line_number;
        // No initializer's declares the variable while giving it a default value
        if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type)))
        {
            const auto index = static_cast<uint8_t>(current_chunk->AddConstant(0));
            current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, index);
        }
        else if(type == PrimitiveType::Float.get())
        {
            const auto index = static_cast<uint8_t>(current_chunk->AddConstant(0.0f));
            current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FLOAT, index);
        }
        else if(type == PrimitiveType::Bool.get())
        {
            const auto index = static_cast<uint8_t>(current_chunk->AddConstant(false));
            current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_BOOL, index);
        }
        else if(const auto* array_type = dynamic_cast<const ArrayType*>(type))
        {
            const uint8_t bytes_per_element = array_type->GetElementType()->GetSize();
            current_chunk->WriteInstruction(line, OpCode::OP_ALLOCATE_ARRAY, static_cast<uint16_t>(0), bytes_per_element);
        }
        else if(const auto* struct_type = dynamic_cast<const StructType*>(type))
        {
            current_chunk->WriteInstruction(line, OpCode::OP_ALLOCATE_STRUCT, static_cast<uint16_t>(struct_type->GetHeapSize()), static_cast<uint8_t>(0));
        }
        else
        {
            throw std::runtime_error("Declaring unknown variable type");
        }
    }

    if(scope_depth == 0)
    {
        const uint16_t offset = global_offsets.at(variable_declaration->name.lexeme);
        const auto line = variable_declaration->source_location.line_number;
        
        current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL, offset, static_cast<uint8_t>(type->GetSize()));
    }
    else
    {
        locals.push_back({variable_declaration->name.lexeme, type});
    }
}

void Compiler::CompileFunctionDeclaration(const FunctionDeclaration* function_declaration)
{
    // global offset was populated in the pre-pass
    const uint16_t offset = global_offsets.at(function_declaration->method_signature.name.lexeme);

    std::unique_ptr<Chunk> outer_scope = std::move(current_chunk);
    current_chunk = std::make_unique<Chunk>();
    
    auto prev_locals = std::move(locals);
    locals.clear();
    
    if(function_declaration->receiver)
    {
        auto& receiver = function_declaration->receiver.value();
        locals.push_back({receiver.name.lexeme, receiver.type_info});
    }

    for(const auto& param: function_declaration->method_signature.parameters)
    {
        locals.push_back({param.name.lexeme, param.type_info});
    }

    CompileStatement(function_declaration->body.get());

    const Type* ret_type = function_declaration->method_signature.return_type_info;
    const auto line = function_declaration->body->source_location.line_number;
    
    if(ret_type == PrimitiveType::Void.get())
    {
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN, static_cast<uint8_t>(0));
    }
    else if(ret_type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(ret_type))
    {
        const auto index = current_chunk->AddConstant(0);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN, static_cast<uint8_t>(4));
    }
    else if(ret_type == PrimitiveType::Float.get())
    {
        const auto index = current_chunk->AddConstant(0.0f);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FLOAT, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN, static_cast<uint8_t>(4));
    }
    else if(ret_type == PrimitiveType::Bool.get())
    {
        const auto index = current_chunk->AddConstant(false);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_BOOL, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN, static_cast<uint8_t>(1));
    }

    auto function_object = std::make_unique<FunctionObject>(
        function_declaration->method_signature.name.lexeme,
        function_declaration->method_signature.parameters.size(),
        std::move(current_chunk)
    );

    current_chunk = std::move(outer_scope);
    const auto declaration_index = current_chunk->AddConstant(function_object.get());

    current_chunk->functions.push_back(std::move(function_object));

    // treated as such
    // OP_CONSTANT_FUNCTION function_object_index
    // OP_DEFINE_GLOBAL offset size
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FUNCTION, static_cast<uint8_t>(declaration_index));
    current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL, offset, static_cast<uint8_t>(8));
    
    locals = std::move(prev_locals);
}


void Compiler::CompileIdentifierExpression(const IdentifierExpression* identifier_expression) const
{
    if(!identifier_expression->type_info || dynamic_cast<const ModuleType*>(identifier_expression->type_info))
    {
        return;
    }

    const auto line = identifier_expression->source_location.line_number;
    const Type* type = identifier_expression->type_info;

    if(scope_depth == 0)
    {
        if(!global_offsets.contains(identifier_expression->name)) return;
        const uint16_t offset = global_offsets.at(identifier_expression->name);
        current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, offset, static_cast<uint8_t>(type->GetSize()));
    }
    else if(const int64_t local_index = GetLocalVariableIndex(identifier_expression->name);
        local_index != -1)
    {
        const auto byte_offset = static_cast<uint16_t>(local_index);
        current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(byte_offset), static_cast<uint8_t>(type->GetSize()));
    }
    else
    {
        if(!global_offsets.contains(identifier_expression->name)) return;
        const uint16_t offset = global_offsets.at(identifier_expression->name);
        current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, offset, static_cast<uint8_t>(type->GetSize()));
    }
}

void Compiler::CompileCallExpression(const CallExpression* call_expression)
{
    const auto line = call_expression->source_location.line_number;

    if(call_expression->is_constructor_call)
    {
        const auto* struct_type = dynamic_cast<const StructType*>(call_expression->type_info);
        for(const auto& arg : call_expression->arguments)
        {
            CompileExpression(arg.get());
        }
        const uint8_t from_stack = call_expression->arguments.empty() ? 0 : 1;
        current_chunk->WriteInstruction(
            line, OpCode::OP_ALLOCATE_STRUCT,
            static_cast<uint16_t>(struct_type->GetHeapSize()), from_stack
        );
        return;
    }

    // If it's an Identifier, CompileIdentifierExpression will automatically
    // emit OP_GET_GLOBAL_FUNCTION or OP_GET_LOCAL.
    // If it's an array index, it will emit OP_GET_INDEX.
    CompileExpression(call_expression->callee.get());

    uint16_t arg_bytes = 0;
    for(const auto& arg : call_expression->arguments)
    {
        CompileExpression(arg.get());
        arg_bytes += arg->type_info->GetSize();
    }

    // OP_CALL now takes a 16-bit arg_bytes operand
    current_chunk->WriteInstruction(line, OpCode::OP_CALL, arg_bytes);
}

void Compiler::CompileArrayAssignmentExpression(const AssignmentExpression* assignment_expression, const uint32_t line, const IndexAccess* const index_access)
{
    CompileExpression(index_access->array_expr.get()); // Push array to the VM stack
    CompileExpression(index_access->index_expr.get()); // Push index to the VM stack
    CompileExpression(assignment_expression->value.get());  // Push rhs value to the VM stack

    // The type of the IndexAccess is the element type
    const uint8_t bytes_per_elem = index_access->type_info->GetSize();
    current_chunk->WriteInstruction(line,
                                    OpCode::OP_SET_INDEX,
                                    bytes_per_elem);
}

void Compiler::CompileVariableAssignmentExpression(const AssignmentExpression* assignment_expression, const uint32_t line, const IdentifierExpression* const identifier)
{
    // find the variable
    const Type* type = assignment_expression->type_info;

    CompileExpression(assignment_expression->value.get());
    if(scope_depth == 0)
    {
        const uint16_t offset = global_offsets.at(identifier->name);
        current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL, offset, static_cast<uint8_t>(type->GetSize()));
    }
    // the variable being called/assigned to is a local one
    else if(const int64_t local_index = GetLocalVariableIndex(identifier->name);
        local_index != -1)
    {
        const auto byte_offset = static_cast<uint16_t>(local_index);
        current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL, static_cast<uint16_t>(byte_offset), static_cast<uint8_t>(type->GetSize()));
    }
    else
    {
        const uint16_t offset = global_offsets.at(identifier->name);
        current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL, offset, static_cast<uint8_t>(type->GetSize()));
    }
}

void Compiler::CompilePropertyAssignmentExpression(const AssignmentExpression* assignment_expression, const uint32_t line, const PropertyAccess* const property_access)
{
    CompileExpression(property_access->object_expr.get());
    CompileExpression(assignment_expression->value.get());

    const auto* struct_type = dynamic_cast<const StructType*>(property_access->object_expr->type_info);
    uint16_t byte_offset = 0;
    uint8_t size = 0;

    for (const auto& [name, type] : struct_type->GetFields())
    {
        if(name == property_access->property_name)
        {
            size = type->GetSize();
            break;
        }
        byte_offset += type->GetSize();
    }
    current_chunk->WriteInstruction(line, OpCode::OP_SET_PROPERTY, byte_offset, size);
}

void Compiler::CompileAssignmentExpression(const AssignmentExpression* assignment_expression)
{
    const auto line = assignment_expression->source_location.line_number;
    if(const auto index_access = dynamic_cast<const IndexAccess*>(assignment_expression->target.get()))
    {
        CompileArrayAssignmentExpression(assignment_expression, line, index_access);
    }
    else if(const auto identifier = dynamic_cast<const IdentifierExpression*>(assignment_expression->target.get()))
    {
        CompileVariableAssignmentExpression(assignment_expression, line, identifier);
    }
    else if(const auto property_access = dynamic_cast<const PropertyAccess*>(assignment_expression->target.get()))
    {
        CompilePropertyAssignmentExpression(assignment_expression, line, property_access);
    }
}

void Compiler::CompileArrayLiteral(const ArrayLiteral* array_literal)
{
    // OP_ALLOCATE_ARRAY [2 bytes: element_count] [1 byte: stride]
    const auto* array_type = dynamic_cast<const ArrayType*>(array_literal->type_info);
    const Type* element_type = array_type->GetElementType();
    const uint8_t bytes_per_element = element_type->GetSize();

    const auto num_elements = static_cast<uint16_t>(array_literal->elements.size());
    for(auto& element: array_literal->elements)
    {
        CompileExpression(element.get());
    }

    current_chunk->WriteInstruction(array_literal->source_location.line_number, OpCode::OP_ALLOCATE_ARRAY, num_elements, bytes_per_element);
}

void Compiler::CompileIndexAccess(const IndexAccess* index_access)
{
    // the array they are trying to access
    CompileExpression(index_access->array_expr.get());
    CompileExpression(index_access->index_expr.get()); // allows arr[someFunc()];

    if(const Type* array_expr_type = index_access->array_expr->type_info; array_expr_type == PrimitiveType::String.get())
    {
        current_chunk->WriteInstruction(
            index_access->source_location.line_number,
            OpCode::OP_GET_STRING_CHAR
        );
    }
    else
    {
        const uint8_t bytes_per_element = index_access->type_info->GetSize();
        current_chunk->WriteInstruction(
            index_access->source_location.line_number,
            OpCode::OP_GET_INDEX,
            bytes_per_element
        );
    }
}

void Compiler::CompilePropertyAccess(const PropertyAccess* property_access)
{
    // OP_GET_PROPERTY [2 bytes: byte_offset] [1 byte: size]     | Stack: Pops 8 byte StructObject*, pushes 'size' bytes from offset

    // again if its an enum dont treat it as an expression
    if(property_access->cached_enum_value.has_value())
    {
        const auto index = current_chunk->AddConstant(property_access->cached_enum_value.value());
        current_chunk->WriteInstruction(property_access->source_location.line_number, OpCode::OP_CONSTANT_INT, static_cast<uint8_t>(index));
        return;
    }

    if (auto* mod_type = dynamic_cast<const ModuleType*>(property_access->object_expr->type_info))
    {
        const std::string mangled = std::format("{}::{}", mod_type->GetName(), property_access->property_name);
        if(!global_offsets.contains(mangled)) return;
        const uint16_t offset = global_offsets.at(mangled);
        current_chunk->WriteInstruction(property_access->source_location.line_number, OpCode::OP_GET_GLOBAL, offset, static_cast<uint8_t>(8));
        return;
    }

    CompileExpression(property_access->object_expr.get()); // allowing someFunc().someProperty;

    if(dynamic_cast<const ArrayType*>(property_access->object_expr->type_info) || property_access->object_expr->type_info == PrimitiveType::String.get())
    {
        current_chunk->WriteInstruction(property_access->source_location.line_number, OpCode::OP_GET_LENGTH);
        return;
    }

    const auto* struct_type = dynamic_cast<const StructType*>(property_access->object_expr->type_info);
    uint16_t byte_offset = 0; // from the start of the struct
    uint8_t size = 0; // and how much bytes this property is

    for (const auto& [name, type] : struct_type->GetFields())
    {
        if(name == property_access->property_name)
        {
            size = type->GetSize();
            break;
        }
        byte_offset += type->GetSize();
    }
    current_chunk->WriteInstruction(property_access->source_location.line_number, OpCode::OP_GET_PROPERTY, byte_offset, size);
}

void Compiler::CompileReturnStatement(const ReturnStatement* return_statement)
{
    if(const auto return_stmt_value = return_statement->value.get())
    {
        CompileExpression(return_statement->value.get());
        const Type* type = return_stmt_value->type_info;
        current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN, type->GetSize());
    }
    else
    {
        current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN, static_cast<uint8_t>(0));
    }
}

void Compiler::CompileBodyStatement(const BodyStatement* body_statement)
{
    ++scope_depth;

    const size_t prev_size = locals.size();
    for(const auto& stmt : body_statement->statements)
    {
        CompileStatement(dynamic_cast<const Statement*>(stmt.get()));
    }
    --scope_depth;

    // if prevent variables defined in inner scopes from remaining in the stack of the VM
    for(size_t i = prev_size; i < locals.size(); ++i)
    {
        const Type* type = locals[i].type;
        const auto line = body_statement->source_location.line_number;
        if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1));
        else if(dynamic_cast<const StructType*>(type) || dynamic_cast<const ArrayType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
    }
    locals.resize(prev_size);
}

void Compiler::CompileIfStatement(const IfStatement* if_statement)
{
    // stack contains the true or false condition
    CompileExpression(if_statement->condition.get());

    const auto line = if_statement->source_location.line_number;

    // Don't know yet how far the jump is so a placeholder offset is used, and then gets set later after the body is compiled
    current_chunk->WriteInstruction(line,OpCode::OP_JUMP_IF_FALSE, static_cast<uint16_t>(0));
    // and jumps use 2 bytes for offsets, (design spec by AI)

    // -2 to get the index of the high byte
    const size_t placeholder_if_index = current_chunk->code.size() - 2;
    CompileStatement(if_statement->body.get());

    if(if_statement->else_branch)
    {
        // REMINDER SINCE THERE WAS ISSUE: don't want the if's body block to fall into the else block.
        // So emit an unconditional jump right here to skip over what is about to get compiled
        current_chunk->WriteInstruction(
            if_statement->else_branch->source_location.line_number,
            OpCode::OP_JUMP, static_cast<uint16_t>(0));

        const uint16_t if_jump = current_chunk->code.size() - (placeholder_if_index + 2);
        current_chunk->code[placeholder_if_index] = (if_jump >> 8) & 0xff;
        current_chunk->code[placeholder_if_index + 1] = if_jump & 0xff;


        const size_t placeholder_else_index = current_chunk->code.size() - 2;
        CompileStatement(if_statement->else_branch.get());

        const uint16_t else_jump = current_chunk->code.size() - (placeholder_else_index + 2);
        current_chunk->code[placeholder_else_index] = (else_jump >> 8) & 0xff;
        current_chunk->code[placeholder_else_index + 1] = else_jump & 0xff;
    }
    else
    {
        const uint16_t if_jump = current_chunk->code.size() - (placeholder_if_index + 2);
        current_chunk->code[placeholder_if_index] = (if_jump >> 8) & 0xff; // High byte
        current_chunk->code[placeholder_if_index + 1] = if_jump & 0xff; // Low byte
    }
}

void Compiler::CompileWhileStatement(const WhileStatement* while_statement)
{
    const size_t loop_start = current_chunk->code.size();
    loop_starts.push_back(loop_start);
    loop_local_counts.push_back(locals.size());

    // stack contains the true or false condition
    CompileExpression(while_statement->condition.get());

    // Don't know yet how far the jump is so a placeholder offset is used, and then gets set later after the body is compiled
    current_chunk->WriteInstruction(
        while_statement->source_location.line_number,
        OpCode::OP_JUMP_IF_FALSE,
        static_cast<uint16_t>(0)
    );
    // and jumps use 2 bytes for offsets, (design spec by AI)

    // -2 to get the index of the high byte
    const size_t placeholder_while_index = current_chunk->code.size() - 2;

    CompileStatement(while_statement->body.get());

    // Offset 3 here so that the vm instruction rechecks the condition for the loop
    const uint16_t backward_jump = (current_chunk->code.size() + 3) - loop_start;
    current_chunk->WriteInstruction(
        while_statement->body->source_location.line_number,
        OpCode::OP_LOOP,
        backward_jump
    );

    const uint16_t while_jump = current_chunk->code.size() - (placeholder_while_index + 2);
    current_chunk->code[placeholder_while_index] = (while_jump >> 8) & 0xff; // High byte
    current_chunk->code[placeholder_while_index + 1] = while_jump & 0xff; // Low byte

    for(const auto index : break_placeholders)
    {
        const uint16_t break_jump = current_chunk->code.size() - (index + 2);
        current_chunk->code[index] = (break_jump >> 8) & 0xff;
        current_chunk->code[index + 1] = break_jump & 0xff;
    }
    break_placeholders.clear();
    loop_starts.pop_back();
    loop_local_counts.pop_back();
}

void Compiler::CompileExpressionStatement(const ExpressionStatement* expression_statement)
{
    CompileExpression(expression_statement->expression.get());
    const Type* type = expression_statement->expression->type_info;
    const auto line = expression_statement->source_location.line_number;

    // We only need to pop if it's not a void expression
    if((type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
    else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
    else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1));
}

void Compiler::CompileContinueStatement(const ContinueStatement* continue_statement) const
{
    const size_t loop_start = loop_starts.back();
    const size_t loop_local_count = loop_local_counts.back();
    const auto line = continue_statement->source_location.line_number;

    // Pop any locals that were created inside the loop body before continuing to the next iteration
    for(size_t i = locals.size(); i > loop_local_count; --i)
    {
        if(const Type* type = locals[i - 1].type; type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type))
        {
            current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        }
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1));
        else if(dynamic_cast<const StructType*>(type) || dynamic_cast<const ArrayType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
    }

    const uint16_t distance = (current_chunk->code.size() + 3) - loop_start; // offset 3 to not rerun the loop instruction

    current_chunk->WriteInstruction(
        line, OpCode::OP_LOOP,
        distance
    );
}

void Compiler::CompileBreakStatement(const BreakStatement* break_statement)
{
    const size_t loop_local_count = loop_local_counts.back();
    const auto line = break_statement->source_location.line_number;

    // Pop any locals that were created inside the loop body before jumping out
    for(size_t i = locals.size(); i > loop_local_count; --i)
    {
        const Type* type = locals[i - 1].type;
        if(type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1));
        else if(dynamic_cast<const StructType*>(type) || dynamic_cast<const ArrayType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
    }

    // Don't know the location so leave placeholder, the while populates (reminder jump uses 2 bytes);
    current_chunk->WriteInstruction(line, OpCode::OP_JUMP, static_cast<uint16_t>(0));
    break_placeholders.push_back(current_chunk->code.size() - 2);
}

void Compiler::CompileNativeModuleStatement(const NativeImportStatement* mod_stmt)
{
    current_native_module_path = project_config->ResolvePluginPath(mod_stmt->name.lexeme);
}

void Compiler::CompileNativeFunctionDeclaration(const NativeFunctionDeclaration* native_function_declaration) const

{
    std::string rel_path = std::filesystem::relative(current_native_module_path, project_config->project_root).string();
    if (rel_path.length() > 0 && rel_path[0] != '.' && rel_path[0] != '/')
    {
        rel_path = "./" + rel_path;
    }

    const auto path_index = current_chunk->AddConstant(rel_path);
    const auto name_index = current_chunk->AddConstant(native_function_declaration->original_name);
    
    // get global offset allocated in pre-pass
    const uint16_t offset = global_offsets.at(native_function_declaration->method_signature.name.lexeme);
    
    const auto num_args = native_function_declaration->method_signature.parameters.size();
    const auto return_bytes = native_function_declaration->method_signature.return_type_info->GetSize();

    current_chunk->WriteInstruction(
        native_function_declaration->source_location.line_number,
        OpCode::OP_LOAD_NATIVE,
        static_cast<uint8_t>(path_index),
        static_cast<uint8_t>(name_index),
        offset,
        static_cast<uint8_t>(num_args),
        static_cast<uint8_t>(return_bytes)
    );
}

void Compiler::ForLoopIterableStruct(const ForLoop* for_loop, const uint32_t line, const StructType* struct_type)
{
    constexpr auto var_name = "$_iter";
    // treat the top of the stack as a hidden local variable
    locals.push_back({var_name, struct_type});
    const auto iter_idx = static_cast<uint16_t>(GetLocalVariableIndex(var_name));

    const size_t loop_start = current_chunk->code.size();
    loop_starts.push_back(loop_start);
    loop_local_counts.push_back(locals.size());

    // Call has_next
    const std::string has_next_name = MangleMethodName(std::string(constants::HAS_NEXT_METHOD), struct_type);
    const uint16_t has_next_offset = global_offsets.at(has_next_name);
    current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, has_next_offset, static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(iter_idx), static_cast<uint8_t>(8));
    const uint16_t arg_bytes = struct_type->GetSize();
    current_chunk->WriteInstruction(line, OpCode::OP_CALL, arg_bytes);

    current_chunk->WriteInstruction(line, OpCode::OP_JUMP_IF_FALSE, static_cast<uint16_t>(0));
    // index to leave loop, placeholder
    const size_t placeholder_if_index = current_chunk->code.size() - 2;

    const std::string next_name = MangleMethodName(std::string(constants::NEXT_METHOD), struct_type);
    const uint16_t next_offset = global_offsets.at(next_name);
    current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, next_offset, static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(iter_idx), static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_CALL, arg_bytes);

    const auto* next_method = struct_type->GetMethod(std::string(constants::NEXT_METHOD));
    const Type* return_type = next_method ? next_method->GetReturnType() : PrimitiveType::Int.get();
    const uint8_t return_size = return_type->GetSize();

    locals.push_back({for_loop->iterator_name.lexeme, return_type});

    CompileStatement(for_loop->body.get());

    locals.pop_back();
    current_chunk->WriteInstruction(line, OpCode::OP_POP, return_size);

    // Loop jump (offset + 3 to recheck condition)
    const uint16_t backward_jump = (current_chunk->code.size() + 3) - loop_start;
    current_chunk->WriteInstruction(line, OpCode::OP_LOOP, backward_jump);


    const uint16_t if_jump = current_chunk->code.size() - (placeholder_if_index + 2);
    current_chunk->code[placeholder_if_index] = (if_jump >> 8) & 0xff; // High byte
    current_chunk->code[placeholder_if_index + 1] = if_jump & 0xff; // Low byte

    for(const auto index : break_placeholders)
    {
        const uint16_t break_jump = current_chunk->code.size() - (index + 2);
        current_chunk->code[index] = (break_jump >> 8) & 0xff;
        current_chunk->code[index + 1] = break_jump & 0xff;
    }

    break_placeholders.clear();
    loop_starts.pop_back();
    loop_local_counts.pop_back();

    locals.pop_back(); // Pop the temp _iter created earlier
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
}

void Compiler::ForLoopArray(const ForLoop* for_loop, const uint32_t line, const ArrayType* array)
{
    // The array pointer is already pushed by CompileExpression in CompileForLoop
    constexpr auto arr_variable_name = "$_arr";
    locals.push_back({arr_variable_name, array});
    const auto arr_idx = static_cast<uint16_t>(GetLocalVariableIndex(arr_variable_name));

    // push -1 for the index variable, if its 0 continue keywords just skip through the increment
    const uint8_t minus_one_idx = current_chunk->AddConstant(-1);
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, minus_one_idx);
    
    constexpr auto index_variable_name = "$_arr_index";
    locals.push_back({index_variable_name, PrimitiveType::Int.get()});
    const auto idx_idx = static_cast<uint16_t>(GetLocalVariableIndex(index_variable_name));

    const size_t loop_start = current_chunk->code.size();
    loop_starts.push_back(loop_start);
    loop_local_counts.push_back(locals.size());

    // Increment index first
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(idx_idx), static_cast<uint8_t>(4));
    const uint8_t one_idx = current_chunk->AddConstant(1);
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, one_idx);
    current_chunk->WriteInstruction(line, OpCode::OP_ADD_INT);
    current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL, static_cast<uint16_t>(idx_idx), static_cast<uint8_t>(4));
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));

    // Check condition: _arr_index < _arr.length
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(idx_idx), static_cast<uint8_t>(4));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(arr_idx), static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LENGTH);
    current_chunk->WriteInstruction(line, OpCode::OP_LESS_INT);

    // Jump if condition is false
    current_chunk->WriteInstruction(line, OpCode::OP_JUMP_IF_FALSE, static_cast<uint16_t>(0));
    const size_t placeholder_if_index = current_chunk->code.size() - 2;

    // Load array element: _arr[_arr_index]
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(arr_idx), static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, static_cast<uint16_t>(idx_idx), static_cast<uint8_t>(4));
    const uint8_t stride = array->GetElementType()->GetSize();
    current_chunk->WriteInstruction(line, OpCode::OP_GET_INDEX, stride);

    // register iterator variable that the user has for the loop
    locals.push_back({for_loop->iterator_name.lexeme, array->GetElementType()});

    CompileStatement(for_loop->body.get());

    locals.pop_back();
    if(const Type* elem_type = array->GetElementType(); elem_type == PrimitiveType::Int.get()
        || dynamic_cast<const EnumType*>(elem_type)
        || elem_type == PrimitiveType::Float.get())
    {
        current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));
    }
    else if(elem_type == PrimitiveType::Bool.get() || elem_type == PrimitiveType::Char.get())
    {
        current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1));
    }
    else if(dynamic_cast<const StructType*>(elem_type) || dynamic_cast<const ArrayType*>(elem_type))
    {
        current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
    }

    const uint16_t backward_jump = (current_chunk->code.size() + 3) - loop_start;
    current_chunk->WriteInstruction(line, OpCode::OP_LOOP, backward_jump);

    // Patch jump if false
    const uint16_t if_jump = current_chunk->code.size() - (placeholder_if_index + 2);
    current_chunk->code[placeholder_if_index] = (if_jump >> 8) & 0xff;
    current_chunk->code[placeholder_if_index + 1] = if_jump & 0xff;

    // Patch break statements
    for(const auto index : break_placeholders)
    {
        const uint16_t break_jump = current_chunk->code.size() - (index + 2);
        current_chunk->code[index] = (break_jump >> 8) & 0xff;
        current_chunk->code[index + 1] = break_jump & 0xff;
    }
    break_placeholders.clear();
    loop_starts.pop_back();
    loop_local_counts.pop_back();

    // Clean up hidden variables
    locals.pop_back(); // Pop _arr_index
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));

    locals.pop_back(); // Pop _arr
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
}

void Compiler::ForLoopString(const ForLoop* for_loop, const uint32_t line, PrimitiveType* get)
{
    // The string pointer is already pushed by CompileExpression in CompileForLoop
    constexpr auto str_variable_name = "$_str";
    locals.push_back({str_variable_name, PrimitiveType::String.get()});
    const auto str_idx = static_cast<uint16_t>(GetLocalVariableIndex(str_variable_name));

    // push -1 for the index variable, if its 0 continue keywords just skip through the increment
    const uint8_t minus_one_idx = current_chunk->AddConstant(-1);
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, minus_one_idx);
    
    constexpr auto index_variable_name = "$_str_index";
    locals.push_back({index_variable_name, PrimitiveType::Int.get()});
    const auto idx_idx = static_cast<uint16_t>(GetLocalVariableIndex(index_variable_name));

    const size_t loop_start = current_chunk->code.size();
    loop_starts.push_back(loop_start);
    loop_local_counts.push_back(locals.size());

    // Increment index first
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, idx_idx, static_cast<uint8_t>(4));
    const uint8_t one_idx = current_chunk->AddConstant(1);
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, one_idx);
    current_chunk->WriteInstruction(line, OpCode::OP_ADD_INT);
    current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL, idx_idx, static_cast<uint8_t>(4));
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));

    // Check condition: _str_index < _str.length
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, idx_idx, static_cast<uint8_t>(4));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, str_idx, static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LENGTH);
    current_chunk->WriteInstruction(line, OpCode::OP_LESS_INT);

    // Jump if condition is false
    current_chunk->WriteInstruction(line, OpCode::OP_JUMP_IF_FALSE, static_cast<uint16_t>(0));
    const size_t placeholder_if_index = current_chunk->code.size() - 2;

    // Load string character: _str[_str_index]
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, str_idx, static_cast<uint8_t>(8));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, idx_idx, static_cast<uint8_t>(4));
    current_chunk->WriteInstruction(line, OpCode::OP_GET_STRING_CHAR);

    // register iterator variable that the user has for the loop
    locals.push_back({for_loop->iterator_name.lexeme, PrimitiveType::Char.get()});

    CompileStatement(for_loop->body.get());

    locals.pop_back(); // loop var
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(1)); // char is 1 byte

    const uint16_t backward_jump = (current_chunk->code.size() + 3) - loop_start;
    current_chunk->WriteInstruction(line, OpCode::OP_LOOP, backward_jump);

    // Patch jump if false
    const uint16_t if_jump = current_chunk->code.size() - (placeholder_if_index + 2);
    current_chunk->code[placeholder_if_index] = (if_jump >> 8) & 0xff;
    current_chunk->code[placeholder_if_index + 1] = if_jump & 0xff;

    // Patch break statements
    for(const auto index : break_placeholders)
    {
        const uint16_t break_jump = current_chunk->code.size() - (index + 2);
        current_chunk->code[index] = (break_jump >> 8) & 0xff;
        current_chunk->code[index + 1] = break_jump & 0xff;
    }
    break_placeholders.clear();
    loop_starts.pop_back();
    loop_local_counts.pop_back();

    // Clean up hidden variables
    locals.pop_back(); // Pop _str_index
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(4));

    locals.pop_back(); // Pop _str
    current_chunk->WriteInstruction(line, OpCode::OP_POP, static_cast<uint8_t>(8));
}

void Compiler::CompileForLoop(const ForLoop* for_loop)
{
    const auto line = for_loop->source_location.line_number;
    auto* iterable_type = for_loop->iterable->type_info;

    CompileExpression(for_loop->iterable.get());

    if(const auto* struct_type = dynamic_cast<const StructType*>(iterable_type); struct_type)
    {
        ForLoopIterableStruct(for_loop, line, struct_type);
    }
    else if(const auto* array_type = dynamic_cast<const ArrayType*>(iterable_type); array_type)
    {
        ForLoopArray(for_loop, line, array_type);
    }
    else if(iterable_type == PrimitiveType::String.get())
    {
        ForLoopString(for_loop, line, PrimitiveType::String.get());
    }
}

int64_t Compiler::GetLocalVariableIndex(const std::string& name) const
{
    int64_t offset = 0;
    for(const auto& [local_name, type] : locals)
    {
        if(local_name == name) return offset;
        offset += type->GetSize();
    }
    return -1;
}
void Compiler::CompileSwitchExpression(const SwitchExpression* switch_expression)
{
    const auto line = switch_expression->source_location.line_number;
    
    // 1. Compile the target expression, pushing it onto the stack.
    CompileExpression(switch_expression->target.get());
    
    const Type* target_type = switch_expression->target->type_info;
    const uint8_t target_size = target_type->GetSize();
    
    std::vector<size_t> end_jump_placeholders;
    
    for(const auto& [pattern, result] : switch_expression->branches)
    {
        if (pattern) // Not the default '_' branch
        {
            // Duplicate the target on top of the stack for comparison
            current_chunk->WriteInstruction(line, OpCode::OP_DUP, target_size);

            // Compile the pattern expression
            CompileExpression(pattern.get());

            // Compare target and pattern
            if(target_type == PrimitiveType::Int.get() || dynamic_cast<const EnumType*>(target_type))
            {
                current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_INT);
            }
            else if(target_type == PrimitiveType::Float.get())
            {
                current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_FLOAT);
            }
            else if(target_type == PrimitiveType::Char.get())
            {
                current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_CHAR);
            }
            else if(target_type == PrimitiveType::Bool.get())
            {
                current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_BOOL);
            }
            else if(target_type == PrimitiveType::String.get())
            {
                current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_STRING);
            }

            // If false, jump to the next branch
            current_chunk->WriteInstruction(line, OpCode::OP_JUMP_IF_FALSE, static_cast<uint16_t>(0));
            const size_t next_branch_jump_idx = current_chunk->code.size() - 2;

            // --- MATCHED BODY ---
            // Pop the original target from the stack since we matched
            current_chunk->WriteInstruction(line, OpCode::OP_POP, target_size);
            CompileExpression(result.get());

            // Jump to the end of the switch
            current_chunk->WriteInstruction(line, OpCode::OP_JUMP, static_cast<uint16_t>(0));
            end_jump_placeholders.push_back(current_chunk->code.size() - 2);

            // Patch the next branch jump
            const uint16_t jump_to_next = current_chunk->code.size() - (next_branch_jump_idx + 2);
            current_chunk->code[next_branch_jump_idx] = (jump_to_next >> 8) & 0xff;
            current_chunk->code[next_branch_jump_idx + 1] = jump_to_next & 0xff;
        }
        else // Default '_' branch
        {
            // Pop the original target
            current_chunk->WriteInstruction(line, OpCode::OP_POP, target_size);
            CompileExpression(result.get());
            
            // Jump to end
            current_chunk->WriteInstruction(line, OpCode::OP_JUMP, static_cast<uint16_t>(0));
            end_jump_placeholders.push_back(current_chunk->code.size() - 2);
        }
    }
    
    // Patch all end jumps to point here
    for(const size_t idx : end_jump_placeholders)
    {
        const uint16_t jump_to_end = current_chunk->code.size() - (idx + 2);
        current_chunk->code[idx] = (jump_to_end >> 8) & 0xff;
        current_chunk->code[idx + 1] = jump_to_end & 0xff;
    }
}

void Compiler::CompileCastExpression(const CastExpression* cast_expression)
{
    CompileExpression(cast_expression->left.get());

    const auto line = cast_expression->source_location.line_number;

    // Upcasting to any (boxing)
    if(dynamic_cast<const AnyType*>(cast_expression->type_info))
    {
        const uint32_t src_id = cast_expression->left->type_info->GetTypeId();
        const uint8_t src_size = cast_expression->left->type_info->GetSize();
        current_chunk->WriteInstruction(line, OpCode::OP_BOX_ANY, src_id, src_size);
    }

    // Down casting from any (checking & unboxing)
    else if(dynamic_cast<const AnyType*>(cast_expression->left->type_info))
    {
        const uint32_t target_id = cast_expression->type_info->GetTypeId();
        const uint8_t target_size = cast_expression->type_info->GetSize();
        current_chunk->WriteInstruction(line, OpCode::OP_CAST_CHECK, target_id, target_size);
    }
}

void Compiler::CompileIsExpression(const IsExpression* is_expression)
{
    CompileExpression(is_expression->left.get());
    const auto line = is_expression->source_location.line_number;
    const uint32_t target_id = is_expression->target_type->GetTypeId();
    current_chunk->WriteInstruction(line, OpCode::OP_IS_CHECK, target_id);
}