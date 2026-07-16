#pragma once
#include <cassert>

#include "frontend/Statement.h"
#include "vm/Chunk.h"
#include <memory>
#include <vector>

class Compiler {
public:
    Compiler() = default;
    Chunk Compile(const std::vector<std::unique_ptr<ASTNode>>& statements);

private:

    void CompileStatement(const Statement* statement);



    void CompileExpression(const Expression* expression);
    void CompileLiteral(const RuntimeValue& value, int line);
    void CompileBinaryExpression(const BinaryExpression* binary_expression, int line);
    void CompileUnaryExpression(const UnaryExpression* unary_expression, int line);


    Chunk current_chunk;
};
