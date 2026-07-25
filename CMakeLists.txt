cmake_minimum_required(VERSION 3.28)
project(tinylang)

set(CMAKE_CXX_STANDARD 26)

add_library(tinylang_lib
        src/frontend/Lexer.cpp
        src/frontend/Token.cpp
        src/frontend/Parser.cpp
        src/analysis/SemanticAnalyzer.cpp
        src/analysis/Type.cpp
        src/vm/ConstantValue.cpp
        src/vm/Chunk.cpp
        src/vm/Compiler.cpp
        src/vm/VM.cpp
        src/vm/VMHeap.cpp
        src/vm/PluginLoader.cpp
        src/frontend/ProjectConfig.cpp
)

target_include_directories(tinylang_lib PUBLIC
        include
)

add_executable(tinylang
        main.cpp
)

target_link_libraries(tinylang PRIVATE tinylang_lib)

# --- Interop API ---
add_library(tinylang_api INTERFACE)
target_include_directories(tinylang_api INTERFACE include)

# --- Plugins ---
add_library(std_plugin MODULE src/plugins/std.cpp)
target_link_libraries(std_plugin PRIVATE tinylang_api)

include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.5
)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

target_link_libraries(tinylang_lib PUBLIC magic_enum::magic_enum nlohmann_json::nlohmann_json)

enable_testing()

add_subdirectory(tests)