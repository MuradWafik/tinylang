#include <catch2/catch_test_macros.hpp>
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "analysis/SemanticAnalyzer.h"

static std::expected<void, std::string> Analyze(const std::string_view source)
{
    Lexer lexer;
    auto lex_result = lexer.Lex(source);
    if (!lex_result) return std::unexpected(lex_result.error().message);

    Parser parser(lex_result.value());
    auto parse_result = parser.ParseProgram();
    if (!parse_result) return std::unexpected(parse_result.error());

    SemanticAnalyzer analyzer{nullptr, false};
    return analyzer.Analyze(parse_result.value());
}

TEST_CASE("Semantic: Valid variable declarations")
{
    SECTION("int declaration") {
        auto result = Analyze("var x: int = 42;");
        REQUIRE(result.has_value());
    }

    SECTION("String declaration") {
        auto result = Analyze("var name: String = \"Hello\";");
        REQUIRE(result.has_value());
    }

    SECTION("bool declaration") {
        auto result = Analyze("var flag: bool = true;");
        REQUIRE(result.has_value());
    }

    SECTION("float declaration") {
        auto result = Analyze("var pi: float = 3.14;");
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Semantic: Invalid variable declarations")
{
    SECTION("Type mismatch int = string") {
        auto result = Analyze("var x: int = \"hello\";");
        REQUIRE(!result.has_value());
    }

    SECTION("Type mismatch bool = int") {
        auto result = Analyze("var flag: bool = 42;");
        REQUIRE(!result.has_value());
    }

    SECTION("Unknown type name") {
        auto result = Analyze("var x: Unknown = 42;");
        REQUIRE(!result.has_value());
    }

    SECTION("Variable redefinition") {
        auto result = Analyze("var x: int = 5; var x: int = 10;");
        REQUIRE(!result.has_value());
    }

    SECTION("Undeclared identifier usage") {
        auto result = Analyze("var x: int = y;");
        REQUIRE(!result.has_value());
    }
}

TEST_CASE("Semantic: Binary and Unary Expressions")
{
    SECTION("Valid binary arithmetic") {
        auto result = Analyze("var x: int = 5 + 10 * 2;");
        REQUIRE(result.has_value());
    }

    SECTION("Invalid binary arithmetic (int + string)") {
        auto result = Analyze("var x: int = 5 + \"hello\";");
        REQUIRE(!result.has_value());
    }

    SECTION("String concatenation") {
        auto result = Analyze("var s: String = \"hello \" + \"world\";");
        REQUIRE(result.has_value());
    }

    SECTION("Valid unary negation") {
        auto result = Analyze("var x: bool = !true;");
        REQUIRE(result.has_value());
    }

    SECTION("Invalid unary negation (negating an int)") {
        auto result = Analyze("var x: bool = !5;");
        REQUIRE(!result.has_value());
    }

    SECTION("Valid unary minus") {
        auto result = Analyze("var x: int = -5;");
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Semantic: Block scoping")
{
    SECTION("Variables do not leak out of body blocks") {
        auto result = Analyze(R"(
            if (true) {
                var x: int = 5;
            }
            var y: int = x; // should fail: x is out of scope!
        )");
        REQUIRE(!result.has_value());
    }

    SECTION("Nested block scopes shadow correctly") {
        auto result = Analyze(R"(
            var x: int = 5;
            if (true) {
                var x: String = "shadow"; // allowed in sub-scope
            }
            var y: int = x; // y = 5, should be fine
        )");
        INFO("Semantic Error: " << (result.has_value() ? "" : result.error()));
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Semantic: Control flow break and continue")
{
    SECTION("Break outside loop is invalid") {
        auto result = Analyze("break;");
        REQUIRE(!result.has_value());
    }

    SECTION("Continue outside loop is invalid") {
        auto result = Analyze("continue;");
        REQUIRE(!result.has_value());
    }

    SECTION("Break and continue inside loop is valid") {
        auto result = Analyze(R"(
            while (true) {
                if (false) {
                    continue;
                }
                break;
            }
        )");
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Semantic: Functions and Calls")
{
    SECTION("Valid function and matching call") {
        auto result = Analyze(R"(
            fn add(a: int, b: int) -> int {
                return a + b;
            }
            var x: int = add(5, 10);
        )");
        INFO("Semantic Error: " << (result.has_value() ? "" : result.error()));
        REQUIRE(result.has_value());
    }

    SECTION("Function argument type mismatch") {
        auto result = Analyze(R"(
            fn add(a: int, b: int) -> int {
                return a + b;
            }
            var x: int = add(5, "hello");
        )");
        REQUIRE(!result.has_value());
    }

    SECTION("Function argument count mismatch") {
        auto result = Analyze(R"(
            fn add(a: int, b: int) -> int {
                return a + b;
            }
            var x: int = add(5);
        )");
        REQUIRE(!result.has_value());
    }

    SECTION("Return statement outside function") {
        auto result = Analyze("return 5;");
        REQUIRE(!result.has_value());
    }

    SECTION("Return type mismatch") {
        auto result = Analyze(R"(
            fn get_number() -> int {
                return "not a number";
            }
        )");
        REQUIRE(!result.has_value());
    }

    SECTION("Recursive calls resolve correctly") {
        auto result = Analyze(R"(
            fn fib(n: int) -> int {
                if (n <= 1) {
                    return n;
                }
                return fib(n - 1) + fib(n - 2);
            }
        )");
        INFO("Semantic Error: " << (result.has_value() ? "" : result.error()));
        REQUIRE(result.has_value());
    }
}

TEST_CASE("Semantic: Interface Conformance and Type Strictness")
{
    SECTION("Cannot use Interface as a concrete variable type") {
        auto result = Analyze(R"(
            interface Logger {
                fn log() -> void;
            }
            var x: Logger;
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("Cannot use interface") != std::string::npos);
    }

    SECTION("Cannot use void as a concrete variable type") {
        auto result = Analyze(R"(
            var x: void;
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("Cannot use 'void' as a type") != std::string::npos);
    }

    SECTION("Self-referencing struct is allowed") {
        auto result = Analyze(R"(
            struct Node {
                var next: Node;
                var value: int;
            }
        )");
        INFO("Semantic Error: " << (result.has_value() ? "" : result.error()));
        REQUIRE(result.has_value());
    }

    SECTION("Successful interface implementation") {
        auto result = Analyze(R"(
            interface Iterator {
                fn has_next() -> bool;
                fn next() -> int;
            }

            struct Range {
                var start: int;
                var end: int;
            }
            extend Range : Iterator;

            fn (self: Range) has_next() -> bool {
                return self.start < self.end;
            }

            fn (self: Range) next() -> int {
                var current: int = self.start;
                self.start = self.start + 1;
                return current;
            }
        )");
        INFO("Semantic Error: " << (result.has_value() ? "" : result.error()));
        REQUIRE(result.has_value());
    }

    SECTION("Missing method in interface implementation") {
        auto result = Analyze(R"(
            interface Iterator {
                fn has_next() -> bool;
                fn next() -> int;
            }

            struct Range {
                var start: int;
                var end: int;
            }
            extend Range : Iterator;

            fn (self: Range) has_next() -> bool {
                return true;
            }
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("does not implement") != std::string::npos);
    }

    SECTION("Incorrect return type in interface implementation") {
        auto result = Analyze(R"(
            interface Iterator {
                fn next() -> int;
            }

            struct Range {
 
            }
            extend Range : Iterator;

            fn (self: Range) next() -> float {
                return 0.0;
            }
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("has return type 'float' but interface 'Iterator' expects 'int'") != std::string::npos);
    }

    SECTION("Incorrect parameter type in interface implementation") {
        auto result = Analyze(R"(
            interface Processor {
                fn process(data: int) -> void;
            }

            struct DataProcessor {
 
            }
            extend DataProcessor : Processor;

            fn (self: DataProcessor) process(data: float) -> void { }
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("Parameter 1 of method 'process' on type 'DataProcessor' has type 'float', but interface 'Processor' expects 'int'") != std::string::npos);
    }

    SECTION("Incorrect parameter count in interface implementation") {
        auto result = Analyze(R"(
            interface Processor {
                fn process(data: int) -> void;
            }

            struct DataProcessor {
 
            }
            extend DataProcessor : Processor;

            fn (self: DataProcessor) process() -> void { }
        )");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error().find("has 0 parameters, but interface 'Processor' expects 1") != std::string::npos);
    }
}

