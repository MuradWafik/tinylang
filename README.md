# tinylang

A high-performance, statically-typed toy programming language running on a custom Bytecode Virtual Machine.

## Architecture & Goals
TinyLang is actively being developed with a strict focus on zero-overhead execution and strong compile-time guarantees. 
For a complete and up-to-date look at the language's planned features (including Go-style Receivers, Explicit CRTP Interfaces, and Generics via Monomorphization), please read the [ROADMAP.md](./ROADMAP.md).

## Completed Features
* Custom Lexer & Parser
* Strict Semantic Analyzer (Type Checker)
* Stack-based Bytecode Virtual Machine
* Reference Types (Arrays, Structs, Strings)
* Method Syntax (Go-Style Receivers)
* Compile-Time Traits / Interfaces
* Simple Enums
* Extension Blocks & Primitive Methods
* Collection & Range Iteration (`for ... in`)
* Conservative Mark-and-Sweep Garbage Collector