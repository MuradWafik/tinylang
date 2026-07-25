#include <catch2/catch_test_macros.hpp>

#include "frontend/Lexer.h"
#include "frontend/Token.h"

#include <catch2/catch_tostring.hpp>

namespace Catch
{
template<>
struct StringMaker<TokenType>
{
    static std::string convert(TokenType type)
    {
        return Token::TypeToString(type);
    }

};
}

static std::vector<Token> Lex(const std::string_view source)
{
    Lexer lexer;

    auto result = lexer.Lex(source);

    REQUIRE(result.has_value());

    return result.value();
}

static LexerError LexError(const std::string_view source)
{
    Lexer lexer;

    auto result = lexer.Lex(source);

    REQUIRE_FALSE(result.has_value());

    return result.error();
}

static void Check(
    const Token& token,
    TokenType type,
    std::string_view lexeme)
{
    REQUIRE(token.type == type);
    REQUIRE(token.lexeme == lexeme);
}

using ExpectedToken = std::pair<TokenType, std::string>;

static void CheckTokens(
    const std::vector<Token>& tokens,
    std::initializer_list<ExpectedToken> expected)
{
    REQUIRE(tokens.size() == expected.size() + 1); // + EOF

    size_t i = 0;
    for (auto&& [type, lexeme] : expected)
    {
        Check(tokens[i], type, lexeme);
        ++i;
    }

    Check(tokens.back(), TokenType::EndOfFile, "");
}

TEST_CASE("Lexes identifier")
{
    const auto tokens = Lex("hello");

    REQUIRE(tokens.size() == 2); // identifier + EOF

    Check(tokens[0], TokenType::Identifier, "hello");
    Check(tokens[1], TokenType::EndOfFile, "");
}

TEST_CASE("Lexes keyword")
{
    const auto tokens = Lex("fn");
    Check(tokens[0], TokenType::Fn, "fn");
}

TEST_CASE("Rejects unterminated string")
{
    auto [message, location] = LexError("\"hello");
    REQUIRE(message == "Missing closing quote to string");
}


TEST_CASE("Empty source")
{
    auto tokens = Lex("");
    REQUIRE(tokens.size() == 1);
    Check(tokens[0], TokenType::EndOfFile, "");
}

TEST_CASE("Whitespace only")
{
    auto tokens = Lex("   \t\n   ");
    REQUIRE(tokens.size() == 1);
    Check(tokens[0], TokenType::EndOfFile, "");
}

TEST_CASE("Identifier")
{
    CheckTokens(
        Lex("hello"),
        {
            {TokenType::Identifier, "hello"},
        });
}

TEST_CASE("Identifier containing digits")
{
    CheckTokens(
        Lex("foo123"),
        {
            {TokenType::Identifier, "foo123"},
        });
}

TEST_CASE("Identifier beginning with underscore")
{
    CheckTokens(
        Lex("_value"),
        {
            {TokenType::Identifier, "_value"},
        });
}

TEST_CASE("Keywords")
{
    CheckTokens(
        Lex("fn var int float String"),
        {
            {TokenType::Fn, "fn"},
            {TokenType::Var, "var"},
            {TokenType::IntType, "int"},
            {TokenType::FloatType, "float"},
            {TokenType::StringType, "String"},
        });
}

TEST_CASE("Control flow keywords")
{
    CheckTokens(
        Lex("if else while return break continue"),
        {
            {TokenType::If, "if"},
            {TokenType::Else, "else"},
            {TokenType::While, "while"},
            {TokenType::Return, "return"},
            {TokenType::Break, "break"},
            {TokenType::Continue, "continue"},
        });
}

TEST_CASE("Integer literal")
{
    CheckTokens(
        Lex("42"),
        {
            {TokenType::IntLiteral, "42"},
        });
}

TEST_CASE("Float literal")
{
    CheckTokens(
        Lex("3.14159"),
        {
            {TokenType::FloatLiteral, "3.14159"},
        });
}

TEST_CASE("Reject malformed float")
{
    auto err = LexError("1.2.3");

    REQUIRE(err.message.contains("Multiple dots"));
}

TEST_CASE("String literal")
{
    CheckTokens(
        Lex("\"hello\""),
        {
            {TokenType::StringLiteral, "hello"},
        });
}

TEST_CASE("Empty string")
{
    CheckTokens(
        Lex("\"\""),
        {
            {TokenType::StringLiteral, ""},
        });
}

TEST_CASE("String with spaces")
{
    CheckTokens(
        Lex("\"hello world\""),
        {
            {TokenType::StringLiteral, "hello world"},
        });
}

TEST_CASE("Unterminated string")
{
    auto err = LexError("\"hello");

    REQUIRE(err.message.contains("Missing closing"));
}

TEST_CASE("String containing newline")
{
    auto err = LexError("\"hello\nworld\"");

    REQUIRE(err.message.contains("newline"));
}

TEST_CASE("Escaped newline")
{
    CheckTokens(
        Lex(R"("a\nb")"),
        {
            {TokenType::StringLiteral, "a\nb"},
        });
}

TEST_CASE("Escaped tab")
{
    CheckTokens(
        Lex(R"("a\tb")"),
        {
            {TokenType::StringLiteral, "a\tb"},
        });
}

TEST_CASE("Escaped quote")
{
    CheckTokens(
        Lex(R"("hello\"world")"),
        {
            {TokenType::StringLiteral, "hello\"world"},
        });
}

TEST_CASE("Escaped backslash")
{
    CheckTokens(
        Lex(R"("C:\\Temp")"),
        {
            {TokenType::StringLiteral, R"(C:\Temp)"},
        });
}

TEST_CASE("Invalid escape")
{
    auto [message, location] = LexError(R"("\q")");

    REQUIRE(message.contains("Unexpected escape"));
}

TEST_CASE("Single-character symbols")
{
    CheckTokens(
        Lex("+-*/(){}[],:;"),
        {
            {TokenType::Plus, "+"},
            {TokenType::Minus, "-"},
            {TokenType::Star, "*"},
            {TokenType::Slash, "/"},
            {TokenType::LeftParen, "("},
            {TokenType::RightParen, ")"},
            {TokenType::LeftCurlyBrace, "{"},
            {TokenType::RightCurlyBrace, "}"},
            {TokenType::LeftSquareBracket, "["},
            {TokenType::RightSquareBracket, "]"},
            {TokenType::Comma, ","},
            {TokenType::Colon, ":"},
            {TokenType::Semicolon, ";"},
        });
}

TEST_CASE("Multi-character operators")
{
    CheckTokens(
        Lex("== != <= >= -> && ||"),
        {
            {TokenType::Equal, "=="},
            {TokenType::NotEqual, "!="},
            {TokenType::LessEqual, "<="},
            {TokenType::GreaterEqual, ">="},
            {TokenType::Arrow, "->"},
            {TokenType::AndAnd, "&&"},
            {TokenType::OrOr, "||"},
        });
}

TEST_CASE("Single-character operators")
{
    CheckTokens(
        Lex("= ! < > & |"),
        {
            {TokenType::Assign, "="},
            {TokenType::Negate, "!"},
            {TokenType::Less, "<"},
            {TokenType::Greater, ">"},
            {TokenType::BitAnd, "&"},
            {TokenType::BitOr, "|"},
        });
}

TEST_CASE("Comments are ignored")
{
    CheckTokens(
        Lex(R"(
            var x: int = 5;
            // comment
            var y: int = 10;
        )"),
        {
            {TokenType::Var, "var"},
            {TokenType::Identifier, "x"},
            {TokenType::Colon, ":"},
            {TokenType::IntType, "int"},
            {TokenType::Assign, "="},
            {TokenType::IntLiteral, "5"},
            {TokenType::Semicolon, ";"},
            {TokenType::Var, "var"},
            {TokenType::Identifier, "y"},
            {TokenType::Colon, ":"},
            {TokenType::IntType, "int"},
            {TokenType::Assign, "="},
            {TokenType::IntLiteral, "10"},
            {TokenType::Semicolon, ";"},
        });
}

TEST_CASE("Multiple consecutive comments are ignored")
{
    CheckTokens(
        Lex(R"(
            var x: int = 5;
            // comment 1
            // comment 2
            // comment 3
            var y: int = 10;
        )"),
        {
            {TokenType::Var, "var"},
            {TokenType::Identifier, "x"},
            {TokenType::Colon, ":"},
            {TokenType::IntType, "int"},
            {TokenType::Assign, "="},
            {TokenType::IntLiteral, "5"},
            {TokenType::Semicolon, ";"},
            {TokenType::Var, "var"},
            {TokenType::Identifier, "y"},
            {TokenType::Colon, ":"},
            {TokenType::IntType, "int"},
            {TokenType::Assign, "="},
            {TokenType::IntLiteral, "10"},
            {TokenType::Semicolon, ";"},
        });
}


TEST_CASE("Unexpected symbol")
{
    auto err = LexError("@");

    REQUIRE(err.message.contains("Unexpected symbol"));
}

TEST_CASE("Simple variable declaration")
{
    CheckTokens(
        Lex("var answer: int = 42;"),
        {
            {TokenType::Var, "var"},
            {TokenType::Identifier, "answer"},
            {TokenType::Colon, ":"},
            {TokenType::IntType, "int"},
            {TokenType::Assign, "="},
            {TokenType::IntLiteral, "42"},
            {TokenType::Semicolon, ";"},
        });
}

TEST_CASE("Simple function signature")
{
    CheckTokens(
        Lex("fn Main(args: String[]) -> int"),
        {
            {TokenType::Fn, "fn"},
            {TokenType::Identifier, "Main"},
            {TokenType::LeftParen, "("},
            {TokenType::Identifier, "args"},
            {TokenType::Colon, ":"},
            {TokenType::StringType, "String"},
            {TokenType::LeftSquareBracket, "["},
            {TokenType::RightSquareBracket, "]"},
            {TokenType::RightParen, ")"},
            {TokenType::Arrow, "->"},
            {TokenType::IntType, "int"},
        });
}


TEST_CASE("Source locations")
{
    auto tokens = Lex("var\nx");

    REQUIRE(tokens[0].source_location.line_number == 1);
    REQUIRE(tokens[0].source_location.column == 1);

    REQUIRE(tokens[1].source_location.line_number == 2);
    REQUIRE(tokens[1].source_location.column == 1);
}
