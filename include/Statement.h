#pragma once
#include <memory>

#include "Expression.h"

// Instruction that does something
// can be like a variable declaration,
// or x = x + 1;
// return statement.. while statement...
struct Statement : ASTNode
{};

struct VariableDeclaration : Statement
{
    std::string name;
    std::string type;
    std::unique_ptr<Expression> initializer;
};

struct ReturnStatement : Statement
{
    std::unique_ptr<Expression> value;
};

struct BodyStatement : Statement
{
    std::vector<std::unique_ptr<Statement>> statements;
};

struct WhileStatement : Statement
{
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BodyStatement> body;
};

