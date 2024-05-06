#include <gtest/gtest.h>

// #include "token.h"
// #include "parser.h"
//
// #define GET_SRC(str) (TEST_DIR str)
//
// class ParserTest : public testing::Test {
//
// };
//
// TEST(Tokenizer, Unknown)
// {
//     std::filesystem::path file = GET_SRC("unknown.ty");
//     TokenList toks;
//     ASSERT_TRUE(toks.tokenize(file));
//
//     EXPECT_EQ(toks.size(), 1);
//     EXPECT_EQ(toks.get_type(), TokenType::UNKNOWN) << "Expected unknown token";
// }
