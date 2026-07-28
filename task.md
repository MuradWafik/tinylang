- `[x]` **Phase 4B: Method Syntax (Go-Style Receivers)**
  - `[x]` Update Parser to support receiver declarations (`fn (self: Vector2) add()`).
  - `[x]` Mangled function names to link methods to structs (e.g. `Vector2_add`).
  - `[x]` Update Semantic Analyzer to register the receiver type in the function signature.
  - `[x]` Desugar method calls (`vec.add()`) into regular function calls (`Vector2_add(vec)`) during semantic analysis.
  - `[x]` Fix compiler scope registration so `self` correctly compiles to `OP_GET_LOCAL_OBJECT`.

- `[/]` **Phase 5: For Loops**
  - `[x]` Add `for`, `in`, `range` tokens to Lexer.
  - `[ ]` Parse `for` statement into `ForStatement` AST node (handling both `range` and iterable arrays).
  - `[ ]` Update Semantic Analyzer to type-check `ForStatement` iterators.
  - `[ ]` Desugar `ForStatement` into bytecode using `OP_LOOP` and `OP_JUMP_IF_FALSE` in `Compiler`.
  - `[ ]` Write and verify tests for `for` loops.

- `[x]` **Phase 4B: Simple Enums**
  - `[x]` Add `enum` token to Lexer.
  - `[x]` Parse `enum` declarations in Parser.
  - `[x]` Update Semantic Analyzer to register `EnumType` and validate enum accesses.
  - `[x]` Update Compiler to treat enum variants as `OP_CONSTANT_INT` internally.
  - `[x]` Write and verify tests for Enums.

- `[x]` **Phase 4B: Interfaces (Non-Generic)**
  - `[x]` Add `interface` token to Lexer.
  - `[x]` Update Parser to parse `InterfaceDeclaration` and interface inheritance in `StructDeclaration`.
  - `[x]` Update Semantic Analyzer with `InterfaceType` to enforce method contracts on structs.
  - `[x]` Create `IntIterator` interface and `Range` struct in standard library.
  - `[x]` Write and verify tests for Interfaces.
