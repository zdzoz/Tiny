#include "test_common.h"

#include "parser.h"
#include "token.h"

#if COMPLEX == 1

class ComplexParserTestSuite : public testing::TestWithParam<std::filesystem::path> { };

TEST_P(ComplexParserTestSuite, Files)
{
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(GetParam()));

    Parser p(std::move(toks));
    int errors = p.parse();

    EXPECT_EQ(errors, 0);
}

INSTANTIATE_TEST_SUITE_P(ParserSuite, ComplexParserTestSuite, testing::ValuesIn(getFiles(INTERMEDIATE_TESTS)));

#endif
