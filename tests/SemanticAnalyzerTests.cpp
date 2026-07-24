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

    SemanticAnalyzer analyzer{nullptr};
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
