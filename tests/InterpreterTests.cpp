#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Expression.h"
#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "TreeWalkInterpreter.h"

// Helper function to run code and extract the value of a specific variable
static RuntimeValue RunAndGetVariable(const std::string& source, const std::string& var_name) {

    Lexer lexer{};
    auto lex_result = lexer.Lex(source);
    REQUIRE(lex_result.has_value());


    Parser parser{lex_result.value()};
    auto parse_result = parser.ParseProgram();

    REQUIRE(parse_result.has_value());

    SemanticAnalyzer semantic_analyzer{};
    auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value());
    INFO("Semantic Analysis Error: " << (semantic_analysis_result.has_value() ? "" : semantic_analysis_result.error()));
    REQUIRE(semantic_analysis_result.has_value());


    TreeWalkInterpreter tree_walk_interpreter{semantic_analyzer};
    for(const auto& node: parse_result.value())
    {
        if(auto* stmt = dynamic_cast<Statement*>(node.get())) tree_walk_interpreter.Execute(stmt);
        else if(auto* expr = dynamic_cast<Expression*>(node.get())) tree_walk_interpreter.Evaluate(expr);
    }

    // Evaluate the variable using an IdentifierExpression
    IdentifierExpression var_expr{var_name};
    return tree_walk_interpreter.Evaluate(&var_expr);
}

TEST_CASE("Interpreter - Basic Arithmetic and Variables", "[Interpreter]") {
    SECTION("Integer arithmetic and variable declaration") {
        std::string source = R"(
            var a: int = 10;
            var b: int = 5;
            var c: int = (a * b) + (a / b) - 2;
        )";
        auto val = RunAndGetVariable(source, "c");
        REQUIRE(std::holds_alternative<int>(val));
        REQUIRE(std::get<int>(val) == 50); // (10 * 5) + (10 / 5) - 2 = 50 + 2 - 2 = 50
    }

    SECTION("Float arithmetic") {
        std::string source = R"(
            var x: float = 3.5;
            var y: float = 2.0;
            var z: float = x * y;
        )";
        auto val = RunAndGetVariable(source, "z");
        REQUIRE(std::holds_alternative<float>(val));
        REQUIRE_THAT(std::get<float>(val), Catch::Matchers::WithinRel(7.0f, 0.001f));
    }
    
    SECTION("String concatenation") {
        std::string source = R"(
            var hello: String = "Hello";
            var world: String = " World";
            var msg: String = hello + world;
        )";
        auto val = RunAndGetVariable(source, "msg");
        REQUIRE(std::holds_alternative<std::string>(val));
        REQUIRE(std::get<std::string>(val) == "Hello World");
    }
}

TEST_CASE("Interpreter - Control Flow", "[Interpreter]") {
    SECTION("If statement true branch") {
        std::string source = R"(
            var result: int = 0;
            if (true) {
                result = 1;
            }
        )";
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 1);
    }

    SECTION("If statement else branch") {
        std::string source = R"(
            var result: int = 0;
            if (false) {
                result = 1;
            } else {
                result = 2;
            }
        )";
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 2);
    }
    
    SECTION("If-else if-else chain") {
        std::string source = R"(
            var val: int = 5;
            var result: int = 0;
            if (val > 10) {
                result = 1;
            } else if (val == 5) {
                result = 2;
            } else {
                result = 3;
            }
        )";
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 2);
    }
}

TEST_CASE("Interpreter - Loops", "[Interpreter]") {
    SECTION("While loop execution") {
        std::string source = R"(
            var counter: int = 0;
            while (counter < 5) {
                counter = counter + 1;
            }
        )";
        auto val = RunAndGetVariable(source, "counter");
        REQUIRE(std::get<int>(val) == 5);
    }

    SECTION("While loop with break") {
        std::string source = R"(
            var counter: int = 0;
            while (counter < 10) {
                if (counter == 3) {
                    break;
                }
                counter = counter + 1;
            }
        )";
        auto val = RunAndGetVariable(source, "counter");
        REQUIRE(std::get<int>(val) == 3);
    }

    SECTION("While loop with continue") {
        std::string source = R"(
            var counter: int = 0;
            var sum: int = 0;
            while (counter < 5) {
                counter = counter + 1;
                if (counter == 3) {
                    continue;
                }
                sum = sum + counter;
            }
        )";
        // sum = 1 + 2 + (skip 3) + 4 + 5 = 12
        auto val = RunAndGetVariable(source, "sum");
        REQUIRE(std::get<int>(val) == 12);
    }
}

TEST_CASE("Interpreter - Functions", "[Interpreter]") {
    SECTION("Simple function call and return") {
        std::string source = R"(
            fn add(a: int, b: int) -> int {
                return a + b;
            }
            var result: int = add(3, 4);
        )";
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 7);
    }

    SECTION("Recursive factorial function") {
        std::string source = R"(
            fn factorial(n: int) -> int {
                if (n <= 1) {
                    return 1;
                }
                return n * factorial(n - 1);
            }
            var result: int = factorial(5);
        )";
        // 5! = 120
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 120);
    }

    SECTION("Recursive fibonacci function") {
        std::string source = R"(
            fn fib(n: int) -> int {
                if (n <= 1) {
                    return n;
                }
                return fib(n - 1) + fib(n - 2);
            }
            var result: int = fib(6);
        )";
        // fib(6) = 8
        auto val = RunAndGetVariable(source, "result");
        REQUIRE(std::get<int>(val) == 8);
    }
}
