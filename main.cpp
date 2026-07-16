// #include <iostream>
// #include <print>
//
// #include "utils/FileReader.h"
// #include "frontend/Lexer.h"
// #include "frontend/Parser.h"
// #include "analysis/SemanticAnalyzer.h"
// #include "interpreter/TreeWalkInterpreter.h"
//
//
// void Run(const int argc, char** argv)
// {
//     if(argc < 2)
//     {
//         std::println(std::cerr, "No File Path provided");
//         return;
//     }
//
//     auto FileOpenResult = FileReader::Read(argv[1]);
//     if(!FileOpenResult.has_value())
//     {
//         std::println(std::cerr, "{}", FileOpenResult.error());
//         return;
//     }
//
//     Lexer lexer{};
//     auto LexResult = lexer.Lex(FileOpenResult.value());
//     if(!LexResult)
//     {
//         std::println(std::cerr, "Error Lexing: {}", LexResult.error().message);
//         std::println(std::cerr, "{}", LexResult.error().location);
//         return;
//     }
//
//     Parser parser{LexResult.value()};
//     auto parse_result = parser.ParseProgram();
//
//     if(!parse_result)
//     {
//         std::println(std::cerr, "Error Parsing: {}", parse_result.error());
//         return;
//     }
//
//     SemanticAnalyzer semantic_analyzer{};
//     if(auto semantic_analysis_result = semantic_analyzer.Analyze(parse_result.value()); !semantic_analysis_result)
//     {
//         std::println(std::cerr, "Error in semantic analysis: {}", semantic_analysis_result.error());
//         return;
//     }
//
//     TreeWalkInterpreter tree_walk_interpreter{semantic_analyzer};
//     for(const auto& node: parse_result.value())
//     {
//         if(auto* stmt = dynamic_cast<Statement*>(node.get())) tree_walk_interpreter.Execute(stmt);
//         else if(auto* expr = dynamic_cast<Expression*>(node.get())) tree_walk_interpreter.Evaluate(expr);
//     }
//
// }
// int main(const int argc, char** argv)
// {
//     Run(argc, argv);
//     return 0;
// }


#include "vm/Chunk.h"
#include "vm/OpCode.h"
#include "vm/VM.h"
#include "vm/Compiler.h"
#include "frontend/Expression.h"
#include "frontend/Token.h"

int main() {
    // 1. Manually build the AST: 1.5 + 2.5
    SourceLocation fake_loc{1, 1};
    auto left = std::make_unique<FloatLiteral>(1.5f, fake_loc);
    auto right = std::make_unique<FloatLiteral>(2.5f, fake_loc);
    
    // Setup a PLUS token
    Token plus_token;
    plus_token.type = TokenType::Plus; 
    
    auto binary_expr = std::make_unique<BinaryExpression>(
        std::move(plus_token), 
        std::move(left), 
        std::move(right),
        fake_loc
    );

    // 2. Put it in a vector (since your Compile method takes a vector of ASTNodes)
    std::vector<std::unique_ptr<ASTNode>> ast;
    ast.push_back(std::move(binary_expr));

    // 3. Compile it!
    Compiler compiler;
    Chunk chunk = compiler.Compile(ast);

    // 4. Disassemble the compiled chunk to verify it worked
    chunk.Disassemble("Compiler Math Test");

    // 5. Run it in the VM!
    VM vm;
    vm.Interpret(&chunk);
    
    return 0;
}
