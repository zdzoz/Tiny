#pragma once

#include "ssa.h"
#include "token.h"

class Parser {
public:
    Parser(TokenList&& toks);
    int parse();

    inline const SSA& get_ssa() { return ssa; }

private:
    TokenList toks;
    int error = 0;
    SSA ssa;

    void varDecl();

    void statSequence();
    void statement();

    // statements
    void assignment();
    void funcCall();
    void ifStatement();
    void whileStatement();
    void returnStatement();

    void expression();
    void term();
    void factor();
    void relation();
};
