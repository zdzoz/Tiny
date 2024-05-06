#include "test_common.h"
#include "token.h"

TEST(Tokenizer, Unknown)
{
    std::string s = "&dss";
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(s));

    EXPECT_EQ(toks.size(), 1);
    EXPECT_EQ(toks.get_type(), TokenType::UNKNOWN) << "Expected unknown token";
}

TEST(Tokenizer, temp)
{
    std::string s = "main var x; {let x <- 10 }.";
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(s));

    EXPECT_EQ(toks.size(), 11);
    EXPECT_EQ(toks.get_type(), TokenType::MAIN);
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    toks.eat();
    EXPECT_EQ(toks.get_type(), TokenType::PERIOD);
}

TEST(Tokenizer, Basic)
{
    std::filesystem::path file = GET_INTERMEDIATE("prof_basic.ty");
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(file));

    std::vector<TokenType> val = {
        TokenType::MAIN,
        TokenType::VAR,
        TokenType::ID, // a
        TokenType::COMMA,
        TokenType::ID, // b
        TokenType::COMMA,
        TokenType::ID, // c
        TokenType::COMMA,
        TokenType::ID, // d
        TokenType::COMMA,
        TokenType::ID, // e
        TokenType::SEMI,
        TokenType::LBRACE,
        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::CALL,
        TokenType::ID,
        TokenType::LPAREN,
        TokenType::RPAREN,
        TokenType::SEMI,

        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::SEMI,

        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::SEMI,

        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::PLUS,
        TokenType::ID,
        TokenType::SEMI,

        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::PLUS,
        TokenType::ID,
        TokenType::SEMI,

        TokenType::IF,
        TokenType::ID,
        TokenType::LT,
        TokenType::NUM,
        TokenType::THEN,
        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::PLUS,
        TokenType::ID,
        TokenType::SEMI,
        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::ELSE,
        TokenType::LET,
        TokenType::ID,
        TokenType::ASSIGN,
        TokenType::ID,
        TokenType::FI,
        TokenType::SEMI,

        TokenType::CALL,
        TokenType::ID,
        TokenType::LPAREN,
        TokenType::ID,
        TokenType::RPAREN,

        TokenType::RBRACE,
        TokenType::PERIOD
    };

    ASSERT_EQ(toks.size(), val.size()) << "Expected vectors to be of equal size";

    for (size_t i = 0; i < toks.size(); i++) {
        EXPECT_EQ(toks.get_type(), val[i]) << "Token types differ";
        toks.eat();
    }
}
