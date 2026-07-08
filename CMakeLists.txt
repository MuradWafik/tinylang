cmake_minimum_required(VERSION 3.31)
project(tinylang)

set(CMAKE_CXX_STANDARD 26)

add_executable(tinylang main.cpp
        Lexer.cpp
        Lexer.h
        Token.cpp
        Token.h
        FileReader.cpp
        FileReader.h)
