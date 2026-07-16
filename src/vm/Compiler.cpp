#include "vm/Compiler.h"

Chunk Compiler::Compile(const std::vector<std::unique_ptr<ASTNode>>& statements)
{
    current_chunk = Chunk(); // Reset for a fresh compile

    for(const auto& node: statements)
    {
        if(const auto* stmt = dynamic_cast<Statement*>(node.get())) CompileStatement(stmt);
        else if(const auto* expr = dynamic_cast<Expression*>(node.get())) CompileExpression(expr);
    }
    
    return std::move(current_chunk);
}

void Compiler::CompileStatement(const Statement* statement)
{
    // TODO: Implement
}

void Compiler::CompileExpression(const Expression* expression)
{
    if(const auto float_lit = dynamic_cast<const FloatLiteral*>(expression)) return CompileLiteral(float_lit->value, -1);
    if(const auto int_lit = dynamic_cast<const IntegerLiteral*>(expression)) return CompileLiteral(int_lit->value, -1);
    if(const auto bool_lit = dynamic_cast<const BoolLiteral*>(expression)) return CompileLiteral(bool_lit->value, -1);
    if(const auto string_lit = dynamic_cast<const StringLiteral*>(expression)) return CompileLiteral(string_lit->value, -1);
    if(const auto binary_expr = dynamic_cast<const BinaryExpression*>(expression)) return CompileBinaryExpression(binary_expr, -1);
    if(const auto unary_expr = dynamic_cast<const UnaryExpression*>(expression)) return CompileUnaryExpression(unary_expr, -1);

} 

void Compiler::CompileLiteral(const RuntimeValue& value, const int line)
{
    const int index = current_chunk.AddConstant(value);
    assert(index < 256 && "Too many constants in current scope, overflowing");
    current_chunk.Write(static_cast<uint8_t>(OpCode::OP_CONSTANT), line);
    current_chunk.Write(index, line);
}

void Compiler::CompileBinaryExpression(const BinaryExpression* binary_expression, const int line)
{
    CompileExpression(binary_expression->left.get());
    CompileExpression(binary_expression->right.get());

    if(binary_expression->operator_token.type == TokenType::Plus)
    {
        current_chunk.Write(static_cast<uint8_t>(OpCode::OP_ADD), line);
    }
}

void Compiler::CompileUnaryExpression(const UnaryExpression* unary_expression, const int line)
{
    throw;
}

