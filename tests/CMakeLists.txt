add_executable(
        tinylangTests
        LexerTests.cpp
        ParserTests.cpp
        SemanticAnalyzerTests.cpp
        VMTests.cpp
)

target_link_libraries(
        tinylangTests
        PRIVATE
        tinylang_lib
        Catch2::Catch2WithMain
)

target_compile_definitions(
        tinylangTests
        PRIVATE
        PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

include(CTest)
include(Catch)

catch_discover_tests(tinylangTests)