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

// statement = assignment | funcCall | ifStatement | whileStatement | returnStatement
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

    ssa.set_symbol(tok);

    // add placeholder if current is empty
    if (ssa.get_current_block()->empty())
        ssa.add_instr(InstrType::NONE);
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

// if = “if” relation “then” statSequence [ “else” statSequence ] “fi”
void Parser::ifStatement()
{
    toks.eat(); // IF
    auto dominator = ssa.get_current_block();

    relation();

    auto left = ssa.add_block(true);
    ssa.reverse_block();
    auto right = ssa.add_block(false);
    JoinNodeType join = {};
    join.node = std::make_shared<Block>();
    join.isLeft = std::nullopt;
    ssa.add_symbols_to_block(join);
    ssa.join_stack.emplace_back(std::move(join));

    left->dominator = dominator;
    right->dominator = dominator;
    ssa.join_stack.back().node->dominator = dominator;

    auto& [join_block, isBranchLeft, idToPhi, _] = ssa.join_stack.back();
    assert(left->parent_left == right->parent_left);
    auto parent = left->parent_left;

    if (toks.get_type() != TokenType::THEN) {
        SYN_EXPECTED("THEN");
        return;
    }
    toks.eat(); // then

    ssa.set_current_block(left);
    isBranchLeft = true;

    INFO("[%s] entering left statSequence\n", __func__);
    statSequence();
    INFO("[%s] exiting left statSequence\n", __func__);
    left = ssa.get_current_block();

    isBranchLeft = false;
    auto original_right = right;
    ssa.set_current_block(right);
    ssa.add_instr(InstrType::NONE);
    if (toks.get_type() == TokenType::ELSE) {
        toks.eat(); // else
        ssa.restore_symbol_state();
        INFO("[%s] entering right statSequence\n", __func__);
        statSequence();
        INFO("[%s] exiting right statSequence\n", __func__);
        right = ssa.get_current_block();
    } else {
        ssa.add_instr(InstrType::NONE);
    }

    if (toks.get_type() != TokenType::FI) {
        SYN_EXPECTED("FI");
        return;
    }
    toks.eat(); // FI

    isBranchLeft = std::nullopt;
    // add branch if to left if join_block has instructions
    if (!join_block->empty()) {
        auto current = ssa.get_current_block();
        ssa.set_current_block(left);
        ssa.add_stack(join_block->front().pos);
        ssa.add_instr(InstrType::BRA);
        ssa.set_current_block(current);
    }

    // join
    join_block->parent_left = left;
    join_block->parent_right = right;
    left->right = join_block;
    right->left = join_block;

    ssa.set_current_block(join_block);

    ssa.resolve_phi(idToPhi);
    ssa.commit_phi(idToPhi);

    ssa.resolve_branch(parent, original_right);

    ssa.join_stack.pop_back();
}

// while = “while” relation “do” StatSequence “od”
void Parser::whileStatement()
{
    toks.eat(); // while

    JoinNodeType join = {};
    join.isLeft = false;

    if (ssa.get_current_block()->size() == 1 && ssa.get_current_block()->back().type == InstrType::NONE) {
        join.node = ssa.get_current_block();
    } else {
        join.node = ssa.add_block(true);
    }

    auto exit_block = ssa.add_block(false);
    ssa.add_instr(InstrType::NONE);

    ssa.set_current_block(join.node);
    ssa.add_symbols_to_block(join);
    ssa.resolve_phi(join.idToPhi);
    relation();
    join.whileInfo = ssa.get_cmp();
    ssa.join_stack.emplace_back(std::move(join));
    auto& join_block = ssa.join_stack.back().node;
    auto loop = ssa.add_block(true);

    loop->dominator = ssa.join_stack.back().node;
    exit_block->dominator = ssa.join_stack.back().node;

    if (toks.get_type() != TokenType::DO) {
        SYN_EXPECTED("DO");
        return;
    }
    toks.eat();

    statSequence();
    auto end_block = ssa.get_current_block();
    end_block->entry = join_block;
    ssa.add_stack(join_block->front().pos);
    ssa.add_instr(InstrType::BRA);

    if (toks.get_type() != TokenType::OD) {
        SYN_EXPECTED("OD");
    } else
        toks.eat();
    ssa.set_current_block(exit_block);

    ssa.resolve_phi(ssa.join_stack.back().idToPhi);
    ssa.commit_phi(ssa.join_stack.back().idToPhi);

    ssa.resolve_branch(join_block, exit_block);

    ssa.join_stack.pop_back();
}

// FIX: return
void Parser::returnStatement()
{
    TODO("Implement %s\n", __func__);
}

/// END STATEMENTS ///

// expression = term {(“+” | “-”) term}
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

// term = factor { (“*” | “/”) factor}.
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

// factor = ident | number | “(“ expression “)” | funcCall
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

// relation = expression relOp expression
void Parser::relation()
{
    expression();

    switch (toks.get_type()) {
    case TokenType::EQ: {
        toks.eat(); // eq
        expression();
        ssa.add_instr(InstrType::BNE);
    } break;
    case TokenType::NEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::BEQ);
        break;
    case TokenType::LT:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::BGE);
        break;
    case TokenType::LTEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::BGT);
        break;
    case TokenType::GT:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::BLE);
        break;
    case TokenType::GTEQ:
        toks.eat();
        expression();
        ssa.add_instr(InstrType::BLT);
        break;
    default:
        SYN_EXPECTED("Relation");
        break;
    }
}
