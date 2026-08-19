#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "frontend/Expression.h"
#include "frontend/Lexer.h"
#include "frontend/Parser.h"
#include "analysis/SemanticAnalyzer.h"
#include "project/ModuleRegistry.h"
#include "vm/Compiler.h"
#include "vm/VM.h"

// Helper function to compile and run code in the VM, then extract a global variable
template <typename T>
static T RunAndGetGlobal(const std::string& source, const std::string& var_name) {
    Lexer lexer{};
    auto lex_result = lexer.Lex(source);
    REQUIRE(lex_result.has_value());

    Parser parser{lex_result.value()};
    auto parse_result = parser.ParseProgram();
    INFO("Parser Error: " << (parse_result.has_value() ? "" : parse_result.error()));
    REQUIRE(parse_result.has_value());

    ProjectConfig project_config{PROJECT_ROOT_DIR, "test", "0.1.0", std::nullopt, {}, {}};
    ModuleRegistry registry;
    registry.RegisterModule("main", std::move(parse_result.value()));

    SemanticAnalyzer semantic_analyzer{&project_config, &registry, false};
    auto semantic_analysis_result = semantic_analyzer.AnalyzeAll();
    INFO("Semantic Analysis Error: " << (semantic_analysis_result.has_value() ? "" : semantic_analysis_result.error()));
    REQUIRE(semantic_analysis_result.has_value());

    Compiler compiler{&project_config, &registry};
    auto chunks = compiler.CompileAll("main");

    VM vm;
    vm.StartProgram(chunks, {"main"});

    if constexpr (std::is_same_v<T, std::string>) {
        auto* str_obj = vm.GetGlobal<String*>(compiler.global_offsets.at(var_name));
        return std::string(str_obj->chars, str_obj->length);
    } else {
        return vm.GetGlobal<T>(compiler.global_offsets.at(var_name));
    }
}

TEST_CASE("VM - Basic Arithmetic and Variables", "[VM]") {
    SECTION("Integer arithmetic and variable declaration") {
        std::string source = R"(
            var a: int = 10;
            var b: int = 5;
            var c: int = (a * b) + (a / b) - 2;
        )";
        auto val = RunAndGetGlobal<int32_t>(source, "c");
        REQUIRE(val == 50); // (10 * 5) + (10 / 5) - 2 = 50 + 2 - 2 = 50
    }

    SECTION("Float arithmetic") {
        std::string source = R"(
            var x: float = 3.5;
            var y: float = 2.0;
            var z: float = x * y;
        )";
        auto val = RunAndGetGlobal<std::float32_t>(source, "z");
        REQUIRE_THAT(val, Catch::Matchers::WithinRel(7.0f, 0.001f));
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
        auto val = RunAndGetGlobal<int>(source, "result");
        REQUIRE(val == 1);
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
        auto val = RunAndGetGlobal<int>(source, "result");
        REQUIRE(val == 2);
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
        auto val = RunAndGetGlobal<int>(source, "counter");
        REQUIRE(val == 5);
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
        auto val = RunAndGetGlobal<int>(source, "result");
        REQUIRE(val == 7);
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
        auto val = RunAndGetGlobal<int>(source, "result");
        REQUIRE(val == 120);
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
        auto val = RunAndGetGlobal<int>(source, "result");
        // Result should still be 0 because right side of && was short-circuited
        REQUIRE(val == 0);
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
        auto val = RunAndGetGlobal<int>(source, "result");
        // Result should still be 0 because right side of || was short-circuited
        REQUIRE(val == 0);
    }
}

TEST_CASE("VM - Native Functions", "[VM]") {
    SECTION("Loads and executes native C++ plugin function") {
        std::string source = R"(
            native import std;
            native fn tinylang_clock() -> float;
            var res: float = tinylang_clock();
        )";
        auto val = RunAndGetGlobal<std::float32_t>(source, "res");
        REQUIRE(val == 42.0f);
    }
}

TEST_CASE("VM - Methods", "[VM]") {
    SECTION("Can declare and call methods with Go-style receivers") {
        std::string source = R"(
            struct Vector2 {
                var x: int;
                var y: int;
            }

            fn (self: Vector2) get_x() -> int {
                return self.x;
            }

            var result: int = 0;
            var vec: Vector2;
            vec.x = 42;
            result = vec.get_x();
        )";
        auto val = RunAndGetGlobal<int>(source, "result");
        REQUIRE(val == 42);
    }
    SECTION("Can instantiate structs using default and full constructors") {
        std::string source = R"(
            struct Vector2 {
                var x: int;
                var y: int;
            }

            var vec1 = Vector2(10, 20);
            var res1 = vec1.x + vec1.y;

            var vec2 = Vector2();
            var res2 = vec2.x + vec2.y;
        )";
        auto val1 = RunAndGetGlobal<int>(source, "res1");
        REQUIRE(val1 == 30);

        auto val2 = RunAndGetGlobal<int>(source, "res2");
        REQUIRE(val2 == 0);
    }
}

TEST_CASE("VM - Enums", "[VM]") {
    SECTION("Can declare and use simple enums") {
        std::string source = R"(
            enum Status {
                Pending,
                Approved,
                Rejected
            }

            var p: Status = Status.Pending;
            var a: Status = Status.Approved;
            var r: Status = Status.Rejected;

            var test1: bool = (p == Status.Pending);
            var test2: bool = (a == Status.Approved);
            var test3: bool = (r == Status.Rejected);
        )";
        REQUIRE(RunAndGetGlobal<bool>(source, "test1") == true);
        REQUIRE(RunAndGetGlobal<bool>(source, "test2") == true);
        REQUIRE(RunAndGetGlobal<bool>(source, "test3") == true);
    }

    SECTION("Can explicitly assign integer values to enum variants") {
        std::string source = R"(
            enum Codes {
                Ok = 200,
                NotFound = 404,
                Error = 500
            }

            var o: Codes = Codes.Ok;
            var n: Codes = Codes.NotFound;
            var is_ok: bool = (o == Codes.Ok);
        )";
        REQUIRE(RunAndGetGlobal<bool>(source, "is_ok") == true);
    }
}

TEST_CASE("VM - Advanced Control Flow", "[VM]") {
    SECTION("For loop with array iteration") {
        std::string source = R"(
            var arr: int[] = [10, 20, 30, 40, 50];
            var sum: int = 0;
            for val in arr {
                sum = sum + val;
            }
        )";
        auto val = RunAndGetGlobal<int>(source, "sum");
        REQUIRE(val == 150);
    }

    SECTION("While loop with break and continue") {
        std::string source = R"(
            var sum: int = 0;
            var i: int = 0;
            while (i < 10) {
                i = i + 1;
                if (i == 5) {
                    continue;
                }
                if (i == 8) {
                    break;
                }
                sum = sum + i;
            }
        )";
        // i goes 1, 2, 3, 4, (5 is skipped), 6, 7. Sum = 1 + 2 + 3 + 4 + 6 + 7 = 23
        auto val = RunAndGetGlobal<int>(source, "sum");
        REQUIRE(val == 23);
    }

    SECTION("For loop array with break and continue") {
        std::string source = R"(
            var arr: int[] = [10, 20, 30, 40, 50, 60];
            var sum: int = 0;
            for val in arr {
                if (val == 30) {
                    continue;
                }
                if (val == 50) {
                    break;
                }
                sum = sum + val;
            }
        )";
        // sum = 10 + 20 + 40 = 70
        auto val = RunAndGetGlobal<int>(source, "sum");
        REQUIRE(val == 70);
    }
}

TEST_CASE("VM - Switch Expression", "[VM]") {
    SECTION("Match integer and default fallback") {
        std::string source = R"(
            var status: int = 404;
            var result: int = switch status {
                200 -> 1,
                404 -> 2,
                _   -> 3
            };
        )";
        auto val = RunAndGetGlobal<int32_t>(source, "result");
        REQUIRE(val == 2);
    }
    
    SECTION("Match default fallback") {
        std::string source = R"(
            var status: int = 500;
            var result: int = switch status {
                200 -> 1,
                404 -> 2,
                _   -> 3
            };
        )";
        auto val = RunAndGetGlobal<int32_t>(source, "result");
        REQUIRE(val == 3);
    }
}

TEST_CASE("VM - String Iteration", "[VM]") {
    SECTION("Iterate over string characters") {
        std::string source = R"(
            var str = "Hello";
            var counter = 0;
            for c in str {
                counter = counter + 1;
            }
        )";
        auto val = RunAndGetGlobal<int32_t>(source, "counter");
        REQUIRE(val == 5);
    }
}

TEST_CASE("VM - String Interpolation", "[VM]") {
    SECTION("Interpolate string variables") {
        std::string source = R"(
            var name = "World";
            var msg = $"Hello {name}!";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "Hello World!");
    }

    SECTION("Interpolate multiple expressions and literals") {
        std::string source = R"(
            fn (self: int) ToString() -> String {
                if(self == 10) { return "10"; }
                if(self == 20) { return "20"; }
                if(self == 30) { return "30"; }
                return "0";
            }
            var a = 10;
            var b = 20;
            var res = $"{a} + {b} = {a + b}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "res");
        REQUIRE(val == "10 + 20 = 30");
    }

    SECTION("Interpolate with escaped braces") {
        std::string source = R"(
            fn (self: int) ToString() -> String {
                return "42";
            }
            var x = 42;
            var res = $"{{{x}}}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "res");
        REQUIRE(val == "{42}");
    }

    SECTION("Native interpolate array of ints") {
        std::string source = R"(
            var list = [1, 2, 3];
            var msg = $"Numbers: {list}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "Numbers: [1, 2, 3]");
    }

    SECTION("Native interpolate array of strings with quotes") {
        std::string source = R"(
            var list = ["apple", "banana"];
            var msg = $"Fruits: {list}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "Fruits: [\"apple\", \"banana\"]");
    }

    SECTION("Native array ToString directly") {
        std::string source = R"(
            var list = [true, false, true];
            var msg = list.ToString();
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "[true, false, true]");
    }

    SECTION("Native nested array ToString") {
        std::string source = R"(
            var nested = [[1, 2], [3, 4]];
            var msg = $"Matrix: {nested}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "Matrix: [[1, 2], [3, 4]]");
    }

    SECTION("Default constructed int array expression") {
        std::string source = R"(
            var arr = int[5];
            var msg = arr.ToString();
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "[0, 0, 0, 0, 0]");
    }

    SECTION("Default constructed sized type declaration") {
        std::string source = R"(
            var arr: int[4];
            var msg = arr.ToString();
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "[0, 0, 0, 0]");
    }

    SECTION("Default constructed String array with quotes") {
        std::string source = R"(
            var names = String[3];
            var msg = names.ToString();
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "[\"\", \"\", \"\"]");
    }

    SECTION("Default constructed bool and float arrays") {
        std::string source = R"(
            var flags = bool[3];
            var msg_flags = flags.ToString();
            var nums = float[2];
            var msg_nums = nums.ToString();
        )";
        auto flags_val = RunAndGetGlobal<std::string>(source, "msg_flags");
        auto nums_val = RunAndGetGlobal<std::string>(source, "msg_nums");
        REQUIRE(flags_val == "[false, false, false]");
        REQUIRE(nums_val == "[0, 0]");
    }

    SECTION("Populate sized array in loop") {
        std::string source = R"(
            var n = 5;
            var arr = int[n];
            var i = 0;
            while(i < arr.length) {
                arr[i] = (i + 1) * 10;
                i = i + 1;
            }
            var msg = $"Results: {arr}";
        )";
        auto val = RunAndGetGlobal<std::string>(source, "msg");
        REQUIRE(val == "Results: [10, 20, 30, 40, 50]");
    }
}
