# TinyLang Master Roadmap

This document outlines the strategic progression for TinyLang, moving from a basic scripting interpreter to a professional-grade compiled language ecosystem.

---

## Phase 1: Core Foundation (✅ Completed)
- Lexer & Parser (Custom Grammar)
- Semantic Analyzer (Strict type checking)
- Tree-Walking Interpreter (Control flow, nested scopes, recursive functions)
- Native Function FFI (C++ standard library registry)

---

## Phase 2: The Bytecode Virtual Machine
**Priority:** Highest | **Difficulty:** High

*   **Goal:** Replace the slow Tree-Walking Interpreter with a high-performance, stack-based Virtual Machine (VM).
*   **Components:**
    *   **Bytecode Compiler:** Flattens the AST into a linear array of 8-bit opcodes (e.g., `OP_ADD`, `OP_JUMP`).
    *   **VM Engine:** A tight execution loop that processes instructions and manipulates a raw memory stack.
*   **Strategic Value:** Building the VM *now* guarantees we only have to implement complex features (like Structs and Garbage Collection) once.

## Phase 4: Compound Data & Memory Management
**Priority:** High | **Difficulty:** Medium-High

*   **Goal:** Introduce Reference Types to the language.
*   **Components:**
    *   **Arrays:** `var list: int[] = [1, 2, 3];`
    *   **Structs:** `struct Point { x: int, y: int }`
    *   **Memory Management:** Because Structs and Arrays are dynamically allocated reference types, the VM must manage their lifecycles to prevent memory leaks. 
        *   *Option A (Ref Counting):* Use C++ `std::shared_ptr` under the hood in the VM for automatic, GC-free memory management.
        *   *Option B (Garbage Collection):* Implement a classic Mark-and-Sweep Garbage Collector directly in the VM.

## Phase 4B: Expressive Syntax, Pattern Matching & Destructuring
**Priority:** Medium-High | **Difficulty:** Medium-High

*   **Collection & Range Iteration (`for ... in`):**
    *   **Range Loops:** `for i in range(0, 10)` – Desugared into a high-performance while loop in the compiler or handled via dedicated loop opcodes.
    *   **Collection Iteration:** `for item in array` – Iterates over arrays, strings, and custom collections.
*   **Tuples & Destructuring / Unpacking:**
    *   **Tuples:** Fixed-size, heterogeneous anonymous containers like `var pair: (int, string) = (42, "hello");`.
    *   **Tuple & Struct Unpacking:** 
        *   Tuple destructuring: `var (x, y) = get_coords();`
        *   Struct destructuring: `var { name, age } = user;`
*   **C#-Style Switch Expressions & Pattern Matching:**
    *   Expression-based switch statements returning a value:
        ```tinylang
        var status_code = 200;
        var message = status_code switch {
            200 => "OK",
            404 => "Not Found",
            _   => "Unknown Error"
        };
        ```
    *   **Pattern Matching:** Match on constants, types, tuple contents, and struct fields.
*   **Enums (Simple & Payload / Tagged Unions):**
    *   **Simple Enums (✅ Completed):** `enum Status { Pending, Approved, Rejected }`
    *   **Payload Enums:** Enums that hold data (Rust/Swift style): `enum Shape { Circle(float), Rectangle(float, float) }`.
    *   Pairs seamlessly with pattern-matching switch expressions: `shape switch { Circle(r) => 3.14 * r * r, ... }`.
*   **String Interpolation:**
    *   `$"Hello {user.name}, you have {count} items"` – Evaluated at compile time by desugaring into string concatenation (`+`) or `format()` calls.
*   **Compile-Time Traits / Interfaces (Non-Generic Contracts):**
    *   **Goal:** Define contracts (e.g. `interface IntIterator { fn has_next() -> bool; }`) to guarantee that structs implement required methods **without** any dynamic dispatch or vtable overhead.
    *   **Method Syntax (Go-Style Receivers) (✅ Completed):** Methods are defined independently from the struct data using a receiver parameter: `fn (self: Vector2) to_string() -> string { ... }`.
    *   **Interface Declaration (C#-Style Explicit) (✅ Completed):** Structs must explicitly declare the interfaces they implement to enforce strict compile-time checking: `struct Range : IntIterator { ... }`.
    *   **Implementation (✅ Completed):** 100% verified during Semantic Analysis. The VM executes direct function calls with zero runtime lookup penalty.

## Phase 5: Modules, Native Plugins & Project Configuration
**Priority:** Medium | **Difficulty:** Medium-High

*   **Goal:** Support multi-file projects (`import "math.tiny"`), dynamic native plugins (`native module "plugins/std.so";`), and project-wide configuration.
*   **Project Manifest (`tinylang.json` / `tinylang.toml`):**
    *   Acts as the single source of truth for the project root directory.
    *   Defines search directories, module aliases, and plugin locations.
*   **Path Resolution Policy:**
    *   **Project-Root Relative:** All module and plugin path strings (e.g. `native module "plugins/std.so";`) are strictly resolved relative to the **Project Root** (discovered by ascending until `tinylang.json` / `tinylang.toml` is found), rather than relative to individual source files.
*   **Implementation:** Requires upgrading the Parser to resolve module paths against the project root, parsing imported files into isolated ASTs, and linking their `SymbolTable`s together during Semantic Analysis to prevent naming collisions.

## Phase 6: Generics & Monomorphization
**Priority:** Low | **Difficulty:** Extremely High

*   **Goal:** Allow reusable, type-safe structures like `Array<T>` and generic functions.
*   **Implementation Strategy:** To support the zero-overhead interfaces from Phase 4B, Generics must be implemented via **Monomorphization** (C++ style). 
    *   The Semantic Analyzer creates a fresh, strongly-typed copy of the generic function or struct for every unique type `T` used in the program.
    *   This guarantees that the Bytecode VM executes blazing fast, direct `OP_CALL` instructions without ever needing vtables or dynamic dispatch.

## Phase 7: Tooling (Qt IDE & LSP)
**Priority:** Lowest (Final Polish) | **Difficulty:** Varies

*   **Qt Text Editor (Medium):** Build a bespoke editor for TinyLang using `QSyntaxHighlighter`, directly hooking the C++ TinyLang Lexer into the Qt rendering pipeline for instant, accurate syntax highlighting.
*   **Language Server Protocol (Very High):** Requires rewriting the Parser to support "Error Recovery" (the ability to continue parsing even when the user types broken syntax) and building an asynchronous JSON-RPC server to communicate with VSCode/CLion.
