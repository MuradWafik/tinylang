#include "vm/Compiler.h"

#include <algorithm>
#include <variant>
#include <variant>

std::unique_ptr<Chunk> Compiler::Compile(const std::vector<std::unique_ptr<ASTNode>>& statements)
{
    current_chunk = std::make_unique<Chunk>(); // Reset for a fresh compile
    for(const auto& node: statements)
    {
        if(const auto* stmt = dynamic_cast<Statement*>(node.get())) CompileStatement(stmt);
        else if(const auto* expr = dynamic_cast<Expression*>(node.get())) CompileExpression(expr);
    }

    const int last_line = current_chunk->lines.empty() ? 0 : current_chunk->lines.back();
    current_chunk->Write(OpCode::OP_RETURN, last_line); // hacky fix make it on the last line
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
}

void Compiler::CompileLiteral(const RuntimeValue& value, const uint32_t line) const
{
    const int index = current_chunk->AddConstant(value);
    assert(index < 256 && "Too many constants in current scope, overflowing");
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT, index);
}

void Compiler::CompileLogicalAnd(const BinaryExpression* binary_expression)
{
    CompileExpression(binary_expression->left.get());

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_JUMP_IF_FALSE_PEEK, 0, 0);
    const size_t jump_index = current_chunk->code.size() - 2;

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP);

    CompileExpression(binary_expression->right.get());

    const uint16_t distance = current_chunk->code.size() - jump_index;
    current_chunk->code[jump_index] = (distance >> 8) & 0xff; // High byte
    current_chunk->code[jump_index + 1] = distance & 0xff; // Low byte
}

void Compiler::CompileLogicalOr(const BinaryExpression* binary_expression)
{
    CompileExpression(binary_expression->left.get());

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_JUMP_IF_TRUE_PEEK, 0, 0);
    const size_t jump_index = current_chunk->code.size() - 2;

    current_chunk->WriteInstruction(binary_expression->left->source_location.line_number, OpCode::OP_POP);

    CompileExpression(binary_expression->right.get());

    const uint16_t distance = current_chunk->code.size() - jump_index;
    current_chunk->code[jump_index] = (distance >> 8) & 0xff; // High byte
    current_chunk->code[jump_index + 1] = distance & 0xff; // Low byte
}

void Compiler::CompileBinaryExpression(const BinaryExpression* binary_expression)
{
    const auto operator_token = binary_expression->operator_token.type;
    // have their own optimization of short circuiting so cant precompile left and right sides
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

    switch(binary_expression->operator_token.type)
    {
        case TokenType::Plus: return current_chunk->Write(OpCode::OP_ADD, line);
        case TokenType::Minus: return current_chunk->Write(OpCode::OP_SUBTRACT, line);
        case TokenType::Star: return current_chunk->Write(OpCode::OP_MULTIPLY, line);
        case TokenType::Slash: return current_chunk->Write(OpCode::OP_DIVIDE, line);
        case TokenType::Greater: return current_chunk->Write(OpCode::OP_GREATER, line);
        case TokenType::GreaterEqual: return current_chunk->Write(OpCode::OP_GREATER_EQUAL, line);
        case TokenType::Less: return current_chunk->Write(OpCode::OP_LESS, line);
        case TokenType::LessEqual: return current_chunk->Write(OpCode::OP_LESS_EQUAL, line);
        case TokenType::Equal: return current_chunk->Write(OpCode::OP_EQUAL, line);
        case TokenType::NotEqual: return current_chunk->Write(OpCode::OP_NOT_EQUAL, line);
        default: assert(false && "Unexpectedly reached default case compiling binary expression");
    }
}

void Compiler::CompileUnaryExpression(const UnaryExpression* unary_expression)
{
    CompileExpression(unary_expression->right.get());

    const auto line = unary_expression->source_location.line_number;

    switch(const auto token_type = unary_expression->operator_token.type)
    {
        // in case more are added? not sure how ill treat negative ints
        case TokenType::Negate: return current_chunk->Write(OpCode::OP_NEGATE, line);
        default: assert(false && "Unexpectedly reached default case compiling unary expression");
    }
}

void Compiler::CompileVariableDeclaration(const VariableDeclaration* variable_declaration)
{
    CompileExpression(variable_declaration->initializer.get());
    // After compiling its value is at the top of the stack

    if(scope_depth == 0)
    {
        const int name_index = current_chunk->AddConstant(variable_declaration->name);
        current_chunk->WriteInstruction(variable_declaration->source_location.line_number, OpCode::OP_DEFINE_GLOBAL, name_index);
    }
    else
    {
        locals.push_back(variable_declaration->name);
    }
}

void Compiler::CompileFunctionDeclaration(const FunctionDeclaration* function_declaration)
{
    std::unique_ptr<Chunk> outer_scope = std::move(current_chunk);

    current_chunk = std::make_unique<Chunk>();

    for(const auto& param: function_declaration->parameters)
    {
        locals.push_back(param.name);
    }

    CompileStatement(function_declaration->body.get());
    current_chunk->Write(OpCode::OP_RETURN, function_declaration->body->source_location.line_number);
    // add a return before resetting the chunk to the outer one


    auto function_object = std::make_shared<FunctionObject>(
        function_declaration->name,
        function_declaration->parameters.size(),
        std::move(current_chunk)
    );

    current_chunk = std::move(outer_scope);
    const auto declaration_index = current_chunk->AddConstant(function_object);
    const auto name_index = current_chunk->AddConstant(function_object->name);


    const auto line = function_declaration->source_location.line_number;

    // treated as such
    // OP_CONSTANT function_object_index
    // OP_DEFINE_GLOBAL name_index
    current_chunk->WriteInstruction(line, OpCode::OP_CONSTANT, declaration_index);
    current_chunk->WriteInstruction(line, OpCode::OP_DEFINE_GLOBAL, name_index);
}


void Compiler::CompileIdentifierExpression(const IdentifierExpression* identifier_expression)
{

    const auto line = identifier_expression->source_location.line_number;

    if(scope_depth == 0)
    {
        const int name_index = current_chunk->AddConstant(identifier_expression->name);
        // Where to find it when looking in the VM
        current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, name_index);
    }
    else if(const int64_t local_index = GetLocalVariableIndex(identifier_expression->name);
        local_index != -1)
    {
        current_chunk->WriteInstruction(line, OpCode::OP_GET_LOCAL, local_index);
    }
    else
    {
        const int name_index = current_chunk->AddConstant(identifier_expression->name);
        // Where to find it when looking in the VM
        current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, name_index);
    }
}

void Compiler::CompileCallExpression(const CallExpression* call_expression)
{
    // find the function
    const int name_index = current_chunk->AddConstant(call_expression->function_name);

    const auto line = call_expression->source_location.line_number;
    current_chunk->WriteInstruction(line, OpCode::OP_GET_GLOBAL, name_index);

    // compile the body
    for(const auto& arg : call_expression->arguments)
    {
        CompileExpression(arg.get());
    }

    current_chunk->WriteInstruction(line, OpCode::OP_CALL, call_expression->arguments.size());
}

void Compiler::CompileAssignmentExpression(const AssignmentExpression* assignment_expression)
{
    // find the variable


    const auto line = assignment_expression->source_location.line_number;

    CompileExpression(assignment_expression->value.get());
    if(scope_depth == 0 )
    {
        const auto name_index = current_chunk->AddConstant(assignment_expression->name);
        current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL, name_index);
    }
    // the variable being called/assigned to is a local one
    else if(const int64_t local_index = GetLocalVariableIndex(assignment_expression->name);
        local_index != -1)
    {
        current_chunk->WriteInstruction(line, OpCode::OP_SET_LOCAL, local_index);
    }
    else
    {
        const auto name_index = current_chunk->AddConstant(assignment_expression->name);
        current_chunk->WriteInstruction(line, OpCode::OP_SET_GLOBAL, name_index);
    }
}

void Compiler::CompileReturnStatement(const ReturnStatement* return_statement)
{
    if(auto return_stmt_value = return_statement->value.get())
    {
        CompileExpression(return_statement->value.get());
    }
    else
    {
        // add a null value for when it gets popped of the stack
        current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_NIL);
    }
    current_chunk->WriteInstruction(return_statement->source_location.line_number, OpCode::OP_RETURN);
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
        current_chunk->WriteInstruction(body_statement->source_location.line_number, OpCode::OP_POP);
    }
    locals.resize(prev_size);
}

void Compiler::CompileIfStatement(const IfStatement* if_statement)
{
    // stack contains the true or false condition
    CompileExpression(if_statement->condition.get());

    const int line = if_statement->source_location.line_number;

    // Don't know yet how far the jump is so a placeholder offset is used, and then gets set later after the body is compiled
    current_chunk->WriteInstruction(line,OpCode::OP_JUMP_IF_FALSE, /*high byte=*/0, /*low byte=*/0);
    // and jumps use 2 bytes for offsets, (design spec by ai)

    // -2 to get the index of the high byte
    const size_t placeholder_if_index = current_chunk->code.size() - 2;
    CompileStatement(if_statement->body.get());

    if(if_statement->else_branch)
    {
        // REMINDER SINCE THERE WAS ISSUE: don't want the if's body block to fall into the else block.
        // So emit an unconditional jump right here to skip over what is about to get compiled
        current_chunk->WriteInstruction(
            if_statement->else_branch->source_location.line_number,
            OpCode::OP_JUMP, 0, 0
        );

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
        0, // high byte
        0 // low byte
    );
    // and jumps use 2 bytes for offsets, (design spec by ai)

    // -2 to get the index of the high byte
    const size_t placeholder_while_index = current_chunk->code.size() - 2;

    CompileStatement(while_statement->body.get());

    // Offset 3 here so that the vm instruction rechecks the condition for the loop
    const uint16_t backward_jump = (current_chunk->code.size() + 3) - loop_start;
    current_chunk->WriteInstruction(
        while_statement->body->source_location.line_number,
        OpCode::OP_LOOP,
        (backward_jump >> 8) & 0xff,
        backward_jump & 0xff
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
    current_chunk->WriteInstruction(
        expression_statement->source_location.line_number,
        OpCode::OP_POP
    );
}

void Compiler::CompileContinueStatement(const ContinueStatement* continue_statement) const
{
    const size_t loop_start = loop_starts.back();
    const uint16_t distance =  (current_chunk->code.size() + 3) - loop_start; // offset 3 to not rerun the loop instruction

    current_chunk->WriteInstruction(
        continue_statement->source_location.line_number, OpCode::OP_LOOP,
        (distance >> 8) & 0xff, distance & 0xff);
}

void Compiler::CompileBreakStatement(const BreakStatement* break_statement)
{
    // Don't know the location so leave placeholder, the while populates (reminder jump uses 2 bytes);
    current_chunk->WriteInstruction(break_statement->source_location.line_number, OpCode::OP_JUMP, 0, 0);
    break_placeholders.push_back(current_chunk->code.size() - 2);
}

int64_t Compiler::GetLocalVariableIndex(const std::string& name)
{
    if(const auto it = std::find(locals.rbegin(), locals.rend(), name);
        it != locals.rend())
    {
        return std::distance(it, locals.rend()) - 1;
    }
    return -1;

    assert(false);
}
