#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "frontend/Expression.h"
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "analysis/SemanticAnalyzer.h"
#include "vm/Compiler.h"
#include "vm/VM.h"

// Helper function to compile and run code in the VM, then extract a global variable
static RuntimeValue RunAndGetGlobal(const std::string& source, const std::string& var_name) {
    Lexer lexer{};
    auto lex_result = lexer.Lex(source);
    REQUIRE(lex_result.has_value());

    Parser parser{lex_result.value()};
    auto parse_result = parser.ParseProgram();
    REQUIRE(parse_result.has_value());

    SemanticAnalyzer semantic_analyzer{nullptr};
    auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value());
    INFO("Semantic Analysis Error: " << (semantic_analysis_result.has_value() ? "" : semantic_analysis_result.error()));
    REQUIRE(semantic_analysis_result.has_value());

    Compiler compiler{nullptr};
    auto chunk = compiler.Compile(parse_result.value());
    REQUIRE(chunk != nullptr);

    VM vm{};
    auto vm_result = vm.Interpret(chunk.get());
    REQUIRE(vm_result == InterpretResult::INTERPRET_OK);

    auto val = vm.GetGlobal(var_name);
    REQUIRE(val.has_value());
    return val.value();
}

TEST_CASE("VM - Basic Arithmetic and Variables", "[VM]") {
    SECTION("Integer arithmetic and variable declaration") {
        std::string source = R"(
            var a: int = 10;
            var b: int = 5;
            var c: int = (a * b) + (a / b) - 2;
        )";
        auto val = RunAndGetGlobal(source, "c");
        REQUIRE(std::holds_alternative<int>(val));
        REQUIRE(std::get<int>(val) == 50); // (10 * 5) + (10 / 5) - 2 = 50 + 2 - 2 = 50
    }

    SECTION("Float arithmetic") {
        std::string source = R"(
            var x: float = 3.5;
            var y: float = 2.0;
            var z: float = x * y;
        )";
        auto val = RunAndGetGlobal(source, "z");
        REQUIRE(std::holds_alternative<float>(val));
        REQUIRE_THAT(std::get<float>(val), Catch::Matchers::WithinRel(7.0f, 0.001f));
    }
}

TEST_CASE("VM - Control Flow", "[VM]") {
    SECTION("If statement true branch") {
        std::string source = R"(
            var result: int = 0;
            if (true) {
                result = 1;
            }
        )";
        auto val = RunAndGetGlobal(source, "result");
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
        auto val = RunAndGetGlobal(source, "result");
        REQUIRE(std::get<int>(val) == 2);
    }
}

TEST_CASE("VM - Loops", "[VM]") {
    SECTION("While loop execution") {
        std::string source = R"(
            var counter: int = 0;
            while (counter < 5) {
                counter = counter + 1;
            }
        )";
        auto val = RunAndGetGlobal(source, "counter");
        REQUIRE(std::get<int>(val) == 5);
    }
}

TEST_CASE("VM - Functions", "[VM]") {
    SECTION("Simple function call and return") {
        std::string source = R"(
            fn add(a: int, b: int) -> int {
                return a + b;
            }
            var result: int = add(3, 4);
        )";
        auto val = RunAndGetGlobal(source, "result");
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
        auto val = RunAndGetGlobal(source, "result");
        REQUIRE(std::get<int>(val) == 120);
    }
}

TEST_CASE("VM - Logical Short Circuit", "[VM]") {
    SECTION("Logical AND short circuits") {
        std::string source = R"(
            var result: int = 0;
            fn side_effect() -> bool {
                result = 99;
                return true;
            }
            if (false && side_effect()) {
                // Should not reach
            }
        )";
        auto val = RunAndGetGlobal(source, "result");
        // Result should still be 0 because right side of && was short-circuited
        REQUIRE(std::get<int>(val) == 0);
    }

    SECTION("Logical OR short circuits") {
        std::string source = R"(
            var result: int = 0;
            fn side_effect() -> bool {
                result = 99;
                return false;
            }
            if (true || side_effect()) {
                // Will reach
            }
        )";
        auto val = RunAndGetGlobal(source, "result");
        // Result should still be 0 because right side of || was short-circuited
        REQUIRE(std::get<int>(val) == 0);
    }
}

TEST_CASE("VM - Native Functions", "[VM]") {
    SECTION("Loads and executes native C++ plugin function") {
        std::string source = R"(
            native module "sample_project/plugins/libstd_plugin.so";
            native fn tinylang_clock() -> float;
            var res: float = tinylang_clock();
        )";
        auto val = RunAndGetGlobal(source, "res");
        REQUIRE(std::get<float>(val) == 42.0f);
    }
}
