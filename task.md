- `[x]` **Phase 4A: Methods (Go-Style Receivers)**
  - `[x]` Add `receiver` parameter to `FunctionDeclaration` in `Statement.h`.
  - `[x]` Update `Parser::ParseFunctionDeclaration` to parse `fn (receiver: Type) method_name()`.
  - `[x]` Update `SemanticAnalyzer` to register method names (e.g. `Type_MethodName`) in the global symbol table.
  - `[x]` Update `SemanticAnalyzer` to inject the `receiver` as a local variable when analyzing the method body.
  - `[x]` Desugar method calls (`vec.add()`) in `Compiler` into standard function calls `Type_MethodName(vec)`.


- `[ ]` **Phase 5: For Loops**
  - `[x]` Add `for`, `in`, `range` tokens to Lexer.
  - `[ ]` Parse `for` statement into `ForStatement` AST node (handling both `range` and iterable arrays).
  - `[ ]` Update Semantic Analyzer to type-check `ForStatement` iterators.
  - `[ ]` Desugar `ForStatement` into bytecode using `OP_LOOP` and `OP_JUMP_IF_FALSE` in `Compiler`.
  - `[ ]` Write and verify tests for `for` loops.

- `[ ]` **Phase 4B: Simple Enums**
  - `[ ]` Add `flags` token to Lexer.
  - `[ ]` Define `EnumDeclaration` AST node in `Statement.h` (supporting `is_flags` and optional explicit `int32_t` values).
  - `[ ]` Update `Parser` to parse `[flags] enum Name { Variant [= IntLiteral], ... }`.
  - `[ ]` Update `SemanticAnalyzer` to register `EnumType`. Auto-assign values (0, 1, 2 for normal; 1, 2, 4 for flags) or use explicit values.
  - `[ ]` Update `Compiler` to treat enum variants as `OP_CONSTANT_INT` natively.
  - `[ ]` Write and verify tests for Enums and Flag Enums.
