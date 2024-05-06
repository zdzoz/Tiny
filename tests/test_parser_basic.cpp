#include "test_common.h"

#include "token.h"
#include "parser.h"

std::vector<std::filesystem::path> getFiles(std::string folder);
class BasicParserTestSuite : public testing::TestWithParam<std::filesystem::path> { };

TEST_P(BasicParserTestSuite, Files)
{
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(GetParam()));

    Parser p(std::move(toks));
    int errors = p.parse();

    EXPECT_EQ(errors, 0);
}

INSTANTIATE_TEST_SUITE_P(ParserSuite, BasicParserTestSuite, testing::ValuesIn(getFiles(BASIC_TESTS)));

std::vector<std::filesystem::path> getFiles(std::string folder)
{
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    return files;
}
