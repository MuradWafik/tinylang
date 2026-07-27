#include "vm/Compiler.h"

#include <algorithm>
#include <variant>

std::unique_ptr<Chunk> Compiler::Compile(const std::vector<std::unique_ptr<ASTNode>>& statements)
{
    current_chunk = std::make_unique<Chunk>(); // Reset for a fresh compile
    for(const auto& node: statements)
    {
        if(const auto* stmt = dynamic_cast<Statement*>(node.get())) CompileStatement(stmt);
        else if(const auto* expr = dynamic_cast<Expression*>(node.get())) CompileExpression(expr);
    }

    const auto last_line = current_chunk->lines.empty() ? 0 : current_chunk->lines.back();
    current_chunk->Write(OpCode::OP_RETURN_VOID, last_line); // hacky fix make it on the last line
    return std::move(current_chunk);
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
    if(const auto mod_stmt = dynamic_cast<const NativeModuleStatement*>(statement)) return CompileNativeModuleStatement(mod_stmt);
    if(const auto native_fn_decl = dynamic_cast<const NativeFunctionDeclaration*>(statement)) return CompileNativeFunctionDeclaration(native_fn_decl);
}

void Compiler::CompileExpression(const Expression* expression)
{
    const auto line = expression->source_location.line_number;
    if(const auto float_lit = dynamic_cast<const FloatLiteral*>(expression)) return CompileLiteral(float_lit->value, line);
    if(const auto int_lit = dynamic_cast<const IntegerLiteral*>(expression)) return CompileLiteral(int_lit->value, line);
    if(const auto bool_lit = dynamic_cast<const BoolLiteral*>(expression)) return CompileLiteral(bool_lit->value, line);
    if(const auto string_lit = dynamic_cast<const StringLiteral*>(expression)) return CompileLiteral(string_lit->value, line);
    if(const auto binary_expr = dynamic_cast<const BinaryExpression*>(expression)) return CompileBinaryExpression(binary_expr);
    if(const auto unary_expr = dynamic_cast<const UnaryExpression*>(expression)) return CompileUnaryExpression(unary_expr);
    if(const auto iden_expr = dynamic_cast<const IdentifierExpression*>(expression)) return CompileIdentifierExpression(iden_expr);
    if(const auto call_expr = dynamic_cast<const CallExpression*>(expression)) return CompileCallExpression(call_expr);
    if(const auto asgn_expr = dynamic_cast<const AssignmentExpression*>(expression)) return CompileAssignmentExpression(asgn_expr);
    if(const auto array_lit = dynamic_cast<const ArrayLiteral*>(expression)) return CompileArrayLiteral(array_lit);
    if(const auto index_access = dynamic_cast<const IndexAccess*>(expression)) return CompileIndexAccess(index_access);
    if(const auto property_access = dynamic_cast<const PropertyAccess*>(expression)) return CompilePropertyAccess(property_access);
}

void Compiler::CompileLiteral(const ConstantValue& value, const uint32_t line) const
{
    const auto index = current_chunk->AddConstant(value);
    assert(index < 256 && "Too many constants in current scope, overflowing");
    
    if(std::holds_alternative<int32_t>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, static_cast<uint8_t>(index));
    else if(std::holds_alternative<std::float32_t>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FLOAT, static_cast<uint8_t>(index));
    else if(std::holds_alternative<bool>(value)) current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_BOOL, static_cast<uint8_t>(index));
    else if(std::holds_alternative<std::string>(value)) current_chunk->WriteInstruction(line, OpCode::OP_ALLOCATE_STRING, static_cast<uint8_t>(index));
}

void Compiler::CompileLogicalAnd(const BinaryExpression* binary_expression)
{
    CompileExpression(binary_expression->left.get());

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_JUMP_IF_FALSE_PEEK, static_cast<uint16_t>(0));
    const size_t jump_index = current_chunk->code.size() - 2;

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP_BOOL);

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

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP_BOOL);

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
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_FLOAT);
            if(type == PrimitiveType::String.get()) return current_chunk->WriteInstruction(line, OpCode::OP_ADD_STRING);
            break;
        }
        case TokenType::Minus:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_SUBTRACT_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_SUBTRACT_FLOAT);
            break;
        }
        case TokenType::Star:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_MULTIPLY_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_MULTIPLY_FLOAT);
            break;
        }
        case TokenType::Slash:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_DIVIDE_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_DIVIDE_FLOAT);
            break;
        }
        case TokenType::Modulo:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_MOD_INT);
            break;
        }
        case TokenType::Greater:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_FLOAT);
            break;
        }
        case TokenType::GreaterEqual:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_GREATER_EQUAL_FLOAT);
            break;
        }
        case TokenType::Less:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_FLOAT);
            break;
        }
        case TokenType::LessEqual:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_LESS_EQUAL_FLOAT);
            break;
        }
        case TokenType::Equal:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_FLOAT);
            if(type == PrimitiveType::Bool.get()) return current_chunk->WriteInstruction(line, OpCode::OP_EQUAL_BOOL);
            break;
        }
        case TokenType::NotEqual:
        {
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_INT);
            if(type == PrimitiveType::Float.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_FLOAT);
            if(type == PrimitiveType::Bool.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NOT_EQUAL_BOOL);
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
            if(type == PrimitiveType::Int.get()) return current_chunk->WriteInstruction(line, OpCode::OP_NEGATE_INT);
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
        if(type == PrimitiveType::Int.get())
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
        const auto name_index = static_cast<uint8_t>(current_chunk->AddConstant(variable_declaration->name));
        const auto line = variable_declaration->source_location.line_number;
        
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_INT, name_index);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_FLOAT, name_index);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_BOOL, name_index);
        else if(dynamic_cast<const FunctionType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_FUNCTION, name_index);
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_OBJECT, name_index);
    }
    else
    {
        locals.push_back({variable_declaration->name, type});
    }
}

void Compiler::CompileFunctionDeclaration(const FunctionDeclaration* function_declaration)
{
    std::unique_ptr<Chunk> outer_scope = std::move(current_chunk);
    current_chunk = std::make_unique<Chunk>();
    
    auto prev_locals = std::move(locals);
    locals.clear();
    
    if(function_declaration->receiver)
    {
        auto& receiver = function_declaration->receiver.value();
        locals.push_back({receiver.name, receiver.type_info});
    }

    for(const auto& param: function_declaration->parameters)
    {
        locals.push_back({param.name, param.type_info});
    }

    CompileStatement(function_declaration->body.get());

    const Type* ret_type = function_declaration->return_type_info;
    const auto line = function_declaration->body->source_location.line_number;
    
    if(ret_type == PrimitiveType::Void.get())
    {
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN_VOID);
    }
    else if(ret_type == PrimitiveType::Int.get())
    {
        const auto index = current_chunk->AddConstant(0);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_INT, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN_INT);
    }
    else if(ret_type == PrimitiveType::Float.get())
    {
        const auto index = current_chunk->AddConstant(0.0f);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FLOAT, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN_FLOAT);
    }
    else if(ret_type == PrimitiveType::Bool.get())
    {
        const auto index = current_chunk->AddConstant(false);
        current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_BOOL, static_cast<uint8_t>(index));
        current_chunk->WriteInstruction(line, OpCode::OP_RETURN_BOOL);
    }

    auto function_object = std::make_unique<FunctionObject>(
        function_declaration->name,
        function_declaration->parameters.size(),
        std::move(current_chunk)
    );

    current_chunk = std::move(outer_scope);
    const auto declaration_index = current_chunk->AddConstant(function_object.get());
    const auto name_index = current_chunk->AddConstant(function_object->name);
    current_chunk->functions.push_back(std::move(function_object));

    // treated as such
    // OP_CONSTANT_FUNCTION function_object_index
    // OP_DEFINE_GLOBAL_FUNCTION name_index
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT_FUNCTION, static_cast<uint8_t>(declaration_index));
    current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL_FUNCTION, static_cast<uint8_t>(name_index));
    
    locals = std::move(prev_locals);
}


void Compiler::CompileIdentifierExpression(const IdentifierExpression* identifier_expression) const
{
    const auto line = identifier_expression->source_location.line_number;
    const Type* type = identifier_expression->type_info;

    if(scope_depth == 0)
    {
        const auto name_index = static_cast<uint8_t>(current_chunk->AddConstant(identifier_expression->name));
        // Where to find it when looking in the VM
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_INT, name_index);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_FLOAT, name_index);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_BOOL, name_index);
        else if(dynamic_cast<const FunctionType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_FUNCTION, name_index);
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_OBJECT, name_index);
    }
    else if(const int64_t local_index = GetLocalVariableIndex(identifier_expression->name);
        local_index != -1)
    {
        const auto byte_offset = static_cast<uint16_t>(local_index);
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL_INT, byte_offset);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL_FLOAT, byte_offset);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL_BOOL, byte_offset);
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL_OBJECT, byte_offset);
    }
    else
    {
        const auto name_index = static_cast<uint8_t>(current_chunk->AddConstant(identifier_expression->name));
        // Where to find it when looking in the VM
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_INT, name_index);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_FLOAT, name_index);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_BOOL, name_index);
        else if(dynamic_cast<const FunctionType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_FUNCTION, name_index);
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL_OBJECT, name_index);
    }
}

void Compiler::CompileCallExpression(const CallExpression* call_expression)
{
    const auto line = call_expression->source_location.line_number;

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
        const auto name_index = current_chunk->AddConstant(identifier->name);
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_INT, static_cast<uint8_t>(name_index));
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_FLOAT, static_cast<uint8_t>(name_index));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_BOOL, static_cast<uint8_t>(name_index));
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_OBJECT, static_cast<uint8_t>(name_index));
    }
    // the variable being called/assigned to is a local one
    else if(const int64_t local_index = GetLocalVariableIndex(identifier->name);
        local_index != -1)
    {
        const auto byte_offset = static_cast<uint16_t>(local_index);
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL_INT, static_cast<uint16_t>(byte_offset ));
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL_FLOAT, static_cast<uint16_t>(byte_offset ));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL_BOOL, static_cast<uint16_t>(byte_offset ));
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL_OBJECT, static_cast<uint16_t>(byte_offset ));
    }
    else
    {
        const auto name_index = current_chunk->AddConstant(identifier->name);
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_INT, static_cast<uint8_t>(name_index));
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_FLOAT, static_cast<uint8_t>(name_index));
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_BOOL, static_cast<uint8_t>(name_index));
        else if(dynamic_cast<const ArrayType*>(type) || dynamic_cast<const StructType*>(type)) current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL_OBJECT, static_cast<uint8_t>(name_index));
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
        if (name == property_access->property_name)
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
    else if (const auto identifier = dynamic_cast<const IdentifierExpression*>(assignment_expression->target.get()))
    {
        CompileVariableAssignmentExpression(assignment_expression, line, identifier);
    }
    else if (const auto property_access = dynamic_cast<const PropertyAccess*>(assignment_expression->target.get()))
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

    const uint8_t bytes_per_element = index_access->type_info->GetSize();
    current_chunk->WriteInstruction(
        index_access->source_location.line_number,
        OpCode::OP_GET_INDEX,
        bytes_per_element
    );
}

void Compiler::CompilePropertyAccess(const PropertyAccess* property_access)
{
    // OP_GET_PROPERTY [2 bytes: byte_offset] [1 byte: size]     | Stack: Pops 8 byte StructObject*, pushes 'size' bytes from offset
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
        if (name == property_access->property_name)
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
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN_INT);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN_FLOAT);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN_BOOL);
        else current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN_OBJECT);
    }
    else
    {
        current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN_VOID);
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
        if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_INT);
        else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_FLOAT);
        else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_BOOL);
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
}

void Compiler::CompileExpressionStatement(const ExpressionStatement* expression_statement)
{
    CompileExpression(expression_statement->expression.get());
    const Type* type = expression_statement->expression->type_info;
    const auto line = expression_statement->source_location.line_number;
    
    // We only need to pop if it's not a void expression
    if(type == PrimitiveType::Int.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_INT);
    else if(type == PrimitiveType::Float.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_FLOAT);
    else if(type == PrimitiveType::Bool.get()) current_chunk->WriteInstruction(line, OpCode::OP_POP_BOOL);
}

void Compiler::CompileContinueStatement(const ContinueStatement* continue_statement) const
{
    const size_t loop_start = loop_starts.back();
    const uint16_t distance = (current_chunk->code.size() + 3) - loop_start; // offset 3 to not rerun the loop instruction

    current_chunk->WriteInstruction(
        continue_statement->source_location.line_number, OpCode::OP_LOOP,
        distance
    );
}

void Compiler::CompileBreakStatement(const BreakStatement* break_statement)
{
    // Don't know the location so leave placeholder, the while populates (reminder jump uses 2 bytes);
    current_chunk->WriteInstruction(break_statement->source_location.line_number, OpCode::OP_JUMP, static_cast<uint16_t>(0));
    break_placeholders.push_back(current_chunk->code.size() - 2);
}

void Compiler::CompileNativeModuleStatement(const NativeModuleStatement* mod_stmt)
{
    current_native_module_path = project_config->ResolvePluginPath(mod_stmt->name);
}

void Compiler::CompileNativeFunctionDeclaration(const NativeFunctionDeclaration* native_function_declaration) const
{
    const auto path_index = current_chunk->AddConstant(current_native_module_path.string());
    const auto name_index = current_chunk->AddConstant(native_function_declaration->name);
    const auto num_args = native_function_declaration->parameters.size();
    const auto return_bytes = native_function_declaration->return_type_info->GetSize();

    current_chunk->WriteInstruction(
        native_function_declaration->source_location.line_number,
        OpCode::OP_LOAD_NATIVE,
        static_cast<uint8_t>(path_index),
        static_cast<uint8_t>(name_index),
        static_cast<uint8_t>(num_args),
        return_bytes
    );
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
