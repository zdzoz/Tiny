#include "parser.h"

#define SYN_ERROR(...)                            \
    do {                                          \
        this->error++;                            \
        LOG_ERROR("[SYNTAX ERROR] " __VA_ARGS__); \
    } while (0)

#define SYN_EXPECTED(type_str)                        \
    do {                                              \
        SYN_ERROR("Expected %s, but got ", type_str); \
        PRTOKLN(*toks.get());                         \
    } while (0)

Parser::Parser(TokenList&& toks)
    : toks(std::move(toks))
{
}

// NOTE:
// “” quotes are used to enclose terminal symbols
// | indicates alternatives
// [] indicates optionality
// {} indicates repetition zero or more times
// () indicates precedence grouping

// “main” [ varDecl ] { funcDecl } “{” statSequence “}” “.”
int Parser::parse()
{
    // "main"
    if (toks.get_type() != TokenType::MAIN)
        SYN_EXPECTED("MAIN");
    else
        toks.eat();

    // [ varDecl ]
    if (toks.get_type() == TokenType::VAR)
        varDecl();

    // TODO: { funcDecl }

    // "{"
    if (toks.get_type() != TokenType::LBRACE)
        SYN_EXPECTED("LBRACE");
    else
        toks.eat();

    // statSequence
    statSequence();

    // }
    if (toks.get_type() != TokenType::RBRACE)
        SYN_EXPECTED("RBRACE");
    else
        toks.eat();

    // .
    if (toks.get_type() != TokenType::PERIOD)
        SYN_EXPECTED("PERIOD");
    else
        toks.eat();

    if (toks.remaining() != 0) {
        SYN_ERROR("Expected EOF\n");
    }

    return error;
}

// [ varDecl ]
void Parser::varDecl()
{
    // eat var
    toks.eat();

    SymbolTableType symbols;
    // u64 num_symbols = 0;
    while (toks.get_type() == TokenType::ID) {
        symbols.push_back({ toks.get()->id(), {} });
        toks.eat(); // eat id

        TokenType typ = toks.get_type();
        if (typ != TokenType::COMMA) {
            if (typ != TokenType::SEMI)
                SYN_EXPECTED("COMMA");
            break;
        }
        toks.eat();
    }
    ssa.add_symbols(std::move(symbols));

    if (toks.get_type() != TokenType::SEMI) {
        SYN_EXPECTED("SEMI");
        return;
    }
    toks.eat();
}

// statement = assignment | funcCall | ifStatement | whileStatement | returnStatement
void Parser::statSequence()
{
    do {
        statement();
        if (toks.get_type() == TokenType::SEMI)
            toks.eat();
        else
            break;
    } while (1);
}

void Parser::statement()
{
    switch (toks.get_type()) {
    case TokenType::LET:
        assignment();
        break;
    case TokenType::CALL:
        funcCall();
        ssa.clear_stack();
        break;
    case TokenType::IF:
        ifStatement();
        break;
    case TokenType::WHILE:
        whileStatement();
        break;
    case TokenType::RET:
        returnStatement();
        break;
    default:
        break;
    }
}

/// STATEMENTS ///

// assignment = “let” ident “<-” expression
void Parser::assignment()
{
    // let
    toks.eat();

    // id
    if (toks.get_type() != TokenType::ID) {
        SYN_EXPECTED("ID");
        return;
    }
    auto tok = toks.get();
    toks.eat();

    // assign
    if (toks.get_type() != TokenType::ASSIGN) {
        SYN_EXPECTED("ASSIGN");
        return;
    }
    toks.eat();

    expression();
    // ssa.set_symbol(tok, ssa.get_last_pos());
    ssa.set_symbol(tok);
}

// funcCall = “call” ident [2 “(“ [expression { “,” expression } ] “)” ]
void Parser::funcCall()
{
    toks.eat(); // eat CALL

    if (toks.get_type() != TokenType::ID) {
        SYN_EXPECTED("ID");
    }
    auto val = toks.get();
    toks.eat(); // ID

    if (toks.get_type() == TokenType::LPAREN) {
        toks.eat(); // LPAREN

        // args
        do {
            expression();
            if (toks.get_type() == TokenType::COMMA) {
                toks.eat(); // COMMA
                if (toks.get_type() == TokenType::RPAREN) {
                    SYN_EXPECTED("Expression");
                    return;
                }
            }
        } while (toks.get_type() != TokenType::RPAREN);
        toks.eat(); // RPAREN
    }

    // FIX: resolve function (add user functions)
    if (ssa.resolve_symbol(val)) {
        // USER DEFINED FUNCTION
        // ssa.add_instr(); // TODO: Jump
    }
}

// FIX: if = “if” relation “then” statSequence [ “else” statSequence ] “fi”
void Parser::ifStatement()
{
    toks.eat(); // IF

    relation();

    ssa.add_block(true);
    auto left = ssa.reverse_block();
    auto right = ssa.add_block(false);
    auto join_block = std::make_shared<Block>();

    if (toks.get_type() != TokenType::THEN) {
        SYN_EXPECTED("THEN");
        return;
    }
    toks.eat(); // then

    ssa.set_current_block(left);

    INFO("%s -> entering statsequence\n", __func__);
    statSequence();
    left = ssa.get_current_block();

    ssa.set_current_block(right);
    if (toks.get_type() == TokenType::ELSE) {
        toks.eat(); // else
        statSequence();
        right = ssa.get_current_block();
    }

    if (toks.get_type() != TokenType::FI) {
        SYN_EXPECTED("FI");
        return;
    }
    toks.eat(); // FI

    // PHI

    // join
    join_block->parent_left = left;
    join_block->parent_right = right;
    left->right = join_block;
    right->left = join_block;

    ssa.set_current_block(join_block);
}

// FIX: while = “while” relation “do” StatSequence “od”
void Parser::whileStatement()
{
    toks.eat(); // while

    relation();

    if (toks.get_type() != TokenType::DO) {
        SYN_EXPECTED("DO");
        return;
    }
    toks.eat();

    statSequence();

    if (toks.get_type() != TokenType::OD) {
        SYN_EXPECTED("OD");
        return;
    }
    toks.eat();
}

// TODO: return
void Parser::returnStatement()
{
    TODO("Implement %s\n", __func__);
}

/// END STATEMENTS ///

// FIX: expression = term {(“+” | “-”) term}
void Parser::expression()
{
    term();
    bool loop = true;
    do {
        switch (toks.get_type()) {
        case TokenType::PLUS:
            toks.eat();
            term();
            ssa.add_instr(InstrType::ADD);
            break;
        case TokenType::MIN:
            toks.eat();
            term();
            ssa.add_instr(InstrType::SUB);
            break;
        default:
            loop = false;
            break;
        }
        if (loop)
            ssa.add_stack(ssa.get_last_pos());
    } while (loop);
}

// FIX: term = factor { (“*” | “/”) factor}.
void Parser::term()
{
    factor();
    bool loop = true;
    do {
        switch (toks.get_type()) {
        case TokenType::MUL:
            toks.eat();
            factor();
            ssa.add_instr(InstrType::MUL);
            break;
        case TokenType::DIV:
            toks.eat();
            factor();
            ssa.add_instr(InstrType::DIV);
            break;
        default:
            loop = false;
            break;
        }
        if (loop)
            ssa.add_stack(ssa.get_last_pos());
    } while (loop);
}

// FIX: factor = ident | number | “(“ expression “)” | funcCall
void Parser::factor()
{
    switch (toks.get_type()) {
    case TokenType::ID:
        ssa.resolve_symbol(toks.get());
        toks.eat();
        break;
    case TokenType::NUM:
        ssa.add_const(*toks.get()->val());
        toks.eat();
        break;
    case TokenType::LPAREN: {
        toks.eat();
        expression();
        if (toks.get_type() != TokenType::RPAREN) {
            SYN_EXPECTED("RPAREN");
        } else
            toks.eat();
    } break;
    case TokenType::CALL:
        funcCall();
        break;
    default:
        break;
    }
}

// FIX: relation = expression relOp expression
void Parser::relation()
{
    expression();

    switch (toks.get_type()) {
    case TokenType::EQ:
        toks.eat(); // eq
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    case TokenType::NEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    case TokenType::LT:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    case TokenType::LTEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    case TokenType::GT:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    case TokenType::GTEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::CMP);
        break;
    default:
        SYN_EXPECTED("Relation");
        break;
    }
}
