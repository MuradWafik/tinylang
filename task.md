- `[ ]` **Phase 4A: Methods (Go-Style Receivers)**
  - `[ ]` Add `receiver` parameter to `FunctionDeclaration` in `Statement.h`.
  - `[ ]` Update `Parser::ParseFunctionDeclaration` to parse `fn (receiver: Type) method_name()`.
  - `[ ]` Update `SemanticAnalyzer` to register method names (e.g. `Type_MethodName`) in the global symbol table.
  - `[ ]` Update `SemanticAnalyzer` to inject the `receiver` as a local variable when analyzing the method body.
  - `[ ]` Desugar method calls (`vec.add()`) in `Compiler` into standard function calls `Type_MethodName(vec)`.

- `[ ]` **Phase 5: For Loops**
  - `[x]` Add `for`, `in`, `range` tokens to Lexer.
  - `[ ]` Parse `for` statement into `ForStatement` AST node (handling both `range` and iterable arrays).
  - `[ ]` Update Semantic Analyzer to type-check `ForStatement` iterators.
  - `[ ]` Desugar `ForStatement` into bytecode using `OP_LOOP` and `OP_JUMP_IF_FALSE` in `Compiler`.
  - `[ ]` Write and verify tests for `for` loops.

- `[ ]` **Phase 4B: Simple Enums**
  - `[x]` Add `enum` token to Lexer.
  - `[ ]` Parse `enum` declarations in Parser.
  - `[ ]` Update Semantic Analyzer to register `EnumType` and validate enum accesses.
  - `[ ]` Update Compiler to treat enum variants as `OP_CONSTANT_INT` internally.
  - `[ ]` Write and verify tests for Enums.
