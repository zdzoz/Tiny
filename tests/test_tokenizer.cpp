#include <gtest/gtest.h>

#include "token.h"

#define GET_SRC(str) (TEST_DIR str)

TEST(Tokenizer, Basic)
{
    std::filesystem::path file = GET_SRC("basic.ty");
    std::vector<Token> toks;
    ASSERT_TRUE(Token::tokenize(file, toks));

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
        EXPECT_EQ(toks[i].type(), val[i]) << "Token types differ";
    }
}
