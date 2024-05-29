#pragma once

#include "ssa.h"
#include "token.h"

class Parser {
public:
    Parser(TokenList&& toks);
    int parse();

    inline const std::deque<SSA>& get_ir() { return ssa_stack; }
    inline void generate_dot() const
    {
        for (auto& e : ssa_stack) {
            e.generate_dot();
            std::cout << std::endl;
        }
    }

private:
    TokenList toks;
    int error = 0;
    std::deque<SSA> ssa_stack;
    SSA* ssa;

    FunctionMap functionMap;

    inline SSA& add_ssa()
    {
        ssa_stack.resize(ssa_stack.size() + 1);
        return ssa_stack.back();
    }

    void varDecl();
    void funcDecl();
    void funcBody();

    void statSequence();
    bool statement();

    // statements
    void assignment();
    bool funcCall(); // returns true if isVoid
    void ifStatement();
    void whileStatement();
    void returnStatement();

    void expression();
    void term();
    void factor();
    void relation();
};
