#include <iostream>

#include "token.h"
#include "parser.h"

#define USAGE_MSG "USAGE: tc <file>"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << USAGE_MSG << std::endl;
        return 1;
    }

    std::filesystem::path file = argv[1];
    TokenList toks;
    if (!toks.tokenize(file)) {
        std::cerr << "Failed to open file: " << file << std::endl;
        return 1;
    }

    #ifndef NDEBUG
    toks.show();
    std::cerr << "\n";
    #endif

    Parser p(std::move(toks));
    int errors = p.parse();
    if (errors > 0) {
        std::cerr << "[PARSER] Failed with " << errors << " errors" << std::endl;
        return 1;
    }

    std::cout << p.get_ssa() << std::endl;

    return 0;
}
