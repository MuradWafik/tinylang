cmake_minimum_required(VERSION 3.31)
project(tinylang)

set(CMAKE_CXX_STANDARD 26)

add_library(tinylang_lib
        src/Lexer.cpp
        src/Token.cpp
        src/FileReader.cpp
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
        GIT_TAG v3.8.1
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

add_subdirectory(tests)