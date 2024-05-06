#include <gtest/gtest.h>
#include <filesystem>

#define GET_BASIC(str) (BASIC_TESTS str)
#define GET_INTERMEDIATE(str) (INTERMEDIATE_TESTS str)
#define GET_COMPLEX(str) (COMPLEX_TESTS str)

extern std::vector<std::filesystem::path> getFiles(std::string folder);
