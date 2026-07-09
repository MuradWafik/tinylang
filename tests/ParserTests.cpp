#include <catch2/catch_test_macros.hpp>
#include "Lexer.h"
#include "Parser.h"
#include <print>

static std::expected<std::vector<std::unique_ptr<ASTNode>>, std::string> Parse(const std::string_view source)
{
    Lexer lexer;
    auto lex_result = lexer.Lex(source);
    if (!lex_result) return std::unexpected(lex_result.error().message);

    Parser parser(lex_result.value());
    return parser.ParseProgram();
}

TEST_CASE("Parses simple variable declaration")
{
    auto result = Parse("var x: int = 42;");
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"(VariableDeclaration(name: "x", type: "int", initializer: IntegerLiteral(42)))");
}

TEST_CASE("Parses binary expressions and precedence")
{
    auto result = Parse("var result: bool = x > y && flag;");
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"(VariableDeclaration(name: "result", type: "bool", initializer: Binary Expression, operator(&&), left: Binary Expression, operator(>), left: Identifier, name("x"), right: Identifier, name("y"), right: Identifier, name("flag")))");
}

TEST_CASE("Parses assignment expression")
{
    auto result = Parse("x = x - 1;");
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"(ExpressionStatement(expr: AssignmentExpression(name: "x", value: Binary Expression, operator(-), left: Identifier, name("x"), right: IntegerLiteral(1))))");
}

TEST_CASE("Parses function declaration and body")
{
    auto result = Parse("fn Main(args: String) -> int { return 0; }");
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(FunctionDeclaration(name: "Main", params: [args: String], return: "int", body: BodyStatement([ReturnStatement(value: IntegerLiteral(0))])))ast");
}

TEST_CASE("Parses empty return statement")
{
    auto result = Parse("fn Main() -> void { return; }");
    // Note: This test will fail with your current type name parsing since 'void' is not recognized as a type name.
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(FunctionDeclaration(name: "Main", params: [], return: "void", body: BodyStatement([ReturnStatement(value: void)])))ast");
}

TEST_CASE("Parses while statement")
{
    auto result = Parse("while (x > 0) { x = x - 1; }");
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(WhileStatement(condition: Binary Expression, operator(>), left: Identifier, name("x"), right: IntegerLiteral(0), body: BodyStatement([ExpressionStatement(expr: AssignmentExpression(name: "x", value: Binary Expression, operator(-), left: Identifier, name("x"), right: IntegerLiteral(1)))])))ast");
}

TEST_CASE("Parses simple if statement")
{
    auto result = Parse("if (flag) { Print(x); }");
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(IfStatement(condition: Identifier, name("flag"), body: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [Identifier, name("x")]))])))ast");
}

TEST_CASE("Parses if-else statement")
{
    auto result = Parse("if (flag) { Print(x); } else { Print(y); }");
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(IfStatement(condition: Identifier, name("flag"), then: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [Identifier, name("x")]))]), else: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [Identifier, name("y")]))])))ast");
}

TEST_CASE("Parses if-else-if-else statement")
{
    auto result = Parse("if (x > 0) { Print(1); } else if (x < 0) { Print(2); } else { Print(0); }");
    INFO("Parser Error: " << (result.has_value() ? "" : result.error()));
    REQUIRE(result.has_value());
    auto& ast = result.value();
    REQUIRE(ast.size() == 1);
    REQUIRE(ast[0]->GetTypeString() == R"ast(IfStatement(condition: Binary Expression, operator(>), left: Identifier, name("x"), right: IntegerLiteral(0), then: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [IntegerLiteral(1)]))]), else: IfStatement(condition: Binary Expression, operator(<), left: Identifier, name("x"), right: IntegerLiteral(0), then: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [IntegerLiteral(2)]))]), else: BodyStatement([ExpressionStatement(expr: CallExpression(name: "Print", args: [IntegerLiteral(0)]))]))))ast");
}
