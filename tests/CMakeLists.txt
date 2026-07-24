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

include(CTest)
include(Catch)

catch_discover_tests(tinylangTests)