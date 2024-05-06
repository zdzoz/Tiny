#include "test_common.h"

#include "parser.h"
#include "token.h"

#if INTERMEDIATE == 1

class IntermediateParserTestSuite : public testing::TestWithParam<std::filesystem::path> { };

TEST_P(IntermediateParserTestSuite, Files)
{
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(GetParam()));

    Parser p(std::move(toks));
    int errors = p.parse();

    EXPECT_EQ(errors, 0);
}

INSTANTIATE_TEST_SUITE_P(ParserSuite, IntermediateParserTestSuite, testing::ValuesIn(getFiles(INTERMEDIATE_TESTS)));

#endif
