#include <iostream>

#include "token.h"

#define USAGE_MSG "USAGE: tc <file>"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << USAGE_MSG << std::endl;
        return 1;
    }

    std::filesystem::path file = argv[1];
    std::vector<Token> toks;
    if (!Token::tokenize(file, toks)) {
        std::cerr << "Failed to open file: " << file << std::endl;
        return 1;
    }

    return 0;
}
