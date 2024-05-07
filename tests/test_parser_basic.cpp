#include "test_common.h"

#include "token.h"
#include "parser.h"

std::vector<std::filesystem::path> getFiles(std::string folder);
class BasicParserTest : public testing::TestWithParam<std::filesystem::path> { };

TEST_P(BasicParserTest, Files)
{
    TokenList toks;
    ASSERT_TRUE(toks.tokenize(GetParam()));

    Parser p(std::move(toks));
    int errors = p.parse();

    EXPECT_EQ(errors, 0);
}

TEST(BasicParserTest, Multiply) {

    std::string s = R"(
        main
        var x, y; {
            let x <- 2;
            let y <- 4;
            let x <- x * y;
            call OutputNum(x)
        }.
    )";

    TokenList toks;
    ASSERT_TRUE(toks.tokenize(s));

    Parser p(std::move(toks));
    int errors = p.parse();

    EXPECT_EQ(errors, 0);
}

INSTANTIATE_TEST_SUITE_P(BasicParserSuite, BasicParserTest, testing::ValuesIn(getFiles(BASIC_TESTS)));

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
