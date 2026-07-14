cmake_minimum_required(VERSION 3.28)
project(tinylang)

set(CMAKE_CXX_STANDARD 26)

add_library(tinylang_lib
        src/Lexer.cpp
        src/Token.cpp
        src/FileReader.cpp
        src/Parser.cpp
        src/SemanticAnalyzer.cpp
        src/Type.cpp
        src/TreeWalkInterpreter.cpp
        src/Environment.cpp
        include/Environment.h
        include/NativeFunction.h
)

target_include_directories(tinylang_lib PUBLIC
        include
)

add_executable(tinylang
        main.cpp
)

target_link_libraries(tinylang PRIVATE tinylang_lib)
include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

add_subdirectory(tests)