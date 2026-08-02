# Phase 5: Modules & Precompiled Bytecode - Architecture & Execution Order

This document outlines the concise architecture and the exact step-by-step order you should follow to implement project-wide modules with circular dependency support.

---

## The Architecture (How it fits together)

1. **`Project` (New):** The entry point. It holds the `ProjectConfig` (which locates `tinylang.json`), manages module resolution paths, and holds the `ModuleRegistry`.
2. **`ModuleRegistry` (New):** A centralized dictionary mapping absolute file paths (e.g., `<project_root>/utils/math.tl`) to `ModuleSymbol` objects.
3. **`ModuleSymbol` (New):** Represents a single `.tl` file. It contains the file's parsed `AST` and a dedicated `SymbolTable` holding only its `export`ed variables, functions, and structs.
4. **`BytecodeSerializer` (New):** Converts a VM `Chunk` to a binary `.tlc` file on disk and vice versa.
5. **`VM Module` (Updated):** The VM will no longer run a single global chunk. It will load `VM::Module` structs (which contain a `Chunk` and a `globals` array). The `CallFrame` tracks which module is executing so `OP_GET_GLOBAL` pulls from the correct array.

---

## Order of Execution (Step-by-Step)

To build this without breaking the existing compiler, follow this exact order:

### Step 1: Syntax & Parsing (The Easy Part)
1.  **Lexer/Parser:** Add the `export` and `import` keywords.
2.  **AST:** Create `ExportStatement` and `ImportStatement` AST nodes.
3.  **Native Modules:** Update `native module "std";` to parse as `native module std;` (removing the quotes).

### Step 2: Project Crawling & Parsing (The Discovery Phase)
1.  **Project Root:** Implement logic to find the project root directory (where `tinylang.json` or the main script lives).
2.  **The Crawler:** Write a recursive function that starts at `main.tl`. 
    - It parses `main.tl` into an AST.
    - It looks for `import A.B;` nodes in the AST.
    - It resolves `A.B` to `<project_root>/A/B.tl`, reads it, parses it, and repeats.
    - *Crucial:* It tracks visited files in a `std::unordered_set<std::string>` to prevent infinite loops from circular `import` statements.

### Step 3: Multi-Pass Semantic Analysis (The Brains)
1.  **Pass 1 (Signatures):** Modify `SemanticAnalyzer`. Loop over every AST discovered in Step 2. Register every `export fn`, `export struct`, and `export var` into that file's `ModuleSymbol` inside the `ModuleRegistry`. *(Do not analyze function bodies yet!)*
2.  **Pass 2 (Logic):** Loop over every AST again. This time, analyze function bodies. When you encounter an `import system.math;` statement, look up `math` in the `ModuleRegistry` and bind its `SymbolTable` to the identifier `math` in the current file's scope. This forces the user to type `math.add()` to access it, ensuring complete clarity.

### Step 4: The Virtual Machine Modules (The Engine)
1.  **VM Refactor:** Change the VM so it holds a `std::vector<VM::Module>` instead of a single `Chunk`. 
2.  **CallFrames:** Update `CallFrame` to hold an index/pointer to the `VM::Module` it belongs to.
3.  **Global Opcodes:** Update `OP_GET_GLOBAL`/`OP_SET_GLOBAL` to read from the current `CallFrame`'s module, ensuring global variables are isolated per file.

### Step 5: Bytecode Serialization (The Speedup)
1.  **Serializer:** Write `BytecodeSerializer` to write a `VM::Module` (its `Chunk`, constants, and globals count) to a `.tlc` binary file.
2.  **Deserializer:** Write the logic to read a `.tlc` file back into a `VM::Module`.
3.  **Integration:** Update Step 2 (The Crawler). When resolving `import A.B;`, check if `<project_root>/A/B.tlc` exists and is newer than `B.tl`. If so, skip parsing and Semantic Analysis for that file, and just hand the `.tlc` file directly to the VM!
