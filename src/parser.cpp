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
    , ssa_stack(1)
{
    assert(ssa_stack.size() == 1);
    ssa = &ssa_stack.back();
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

    while (toks.get_type() == TokenType::VOID || toks.get_type() == TokenType::FUNC)
        funcDecl();

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
    while (toks.get_type() == TokenType::ID) {
        symbols[*toks.get()->val()] = { toks.get()->id(), std::nullopt };
        toks.eat(); // eat id

        TokenType typ = toks.get_type();
        if (typ != TokenType::COMMA) {
            if (typ != TokenType::SEMI)
                SYN_EXPECTED("COMMA");
            break;
        }
        toks.eat();
    }
    ssa->add_symbols(std::move(symbols));

    if (toks.get_type() != TokenType::SEMI) {
        SYN_EXPECTED("SEMI");
        return;
    }
    toks.eat();
}

// funcDecl = [ “void” ] “function” ident formalParam “;” funcBody “;” .
void Parser::funcDecl()
{
    bool isVoid = false;
    if (toks.get_type() == TokenType::VOID) {
        isVoid = true;
        toks.eat(); // eat void
    }

    if (toks.get_type() != TokenType::FUNC) {
        SYN_EXPECTED("FUNC");
        return;
    }
    toks.eat(); // eat func

    if (toks.get_type() != TokenType::ID) {
        SYN_EXPECTED("ID");
        return;
    }

    auto& s = add_ssa(); // create function ssa
    s.add_instr(InstrType::NONE); // add temp instr

    assert(toks.get()->val() != std::nullopt);
    s.name = toks.get()->id();
    functionMap[*toks.get()->val()].pos = s.get_first_instr();
    functionMap[*toks.get()->val()].isVoid = isVoid;
    auto& paramCount = functionMap[*toks.get()->val()].paramCount;
    paramCount = 0;

    // swap to function ssa
    ssa = &s;
    ssa->clear_stack();

    toks.eat(); // eat id

    // formalParam = “(“ [ident { “,” ident }] “)”
    if (toks.get_type() != TokenType::LPAREN) {
        SYN_EXPECTED("LPAREN");
        return;
    }
    toks.eat(); // lparen

    // get params
    SymbolTableType symbols;
    while (toks.get_type() == TokenType::ID) {
        ssa->add_stack(paramCount + 1);
        ssa->add_instr(InstrType::GETP);

        symbols[*toks.get()->val()] = { toks.get()->id(), ssa->get_last_pos() };
        paramCount++;

        toks.eat(); // eat id
        if (toks.get_type() != TokenType::COMMA)
            break;
        toks.eat(); // eat comma
    }
    ssa->add_symbols(std::move(symbols));

    if (toks.get_type() != TokenType::RPAREN) {
        SYN_EXPECTED("RPAREN");
        return;
    }
    toks.eat(); // eat rparen

    if (toks.get_type() != TokenType::SEMI) {
        SYN_EXPECTED("SEMI");
        return;
    }
    toks.eat(); // eat semi

    funcBody();

    if (toks.get_type() != TokenType::SEMI) {
        SYN_EXPECTED("SEMI");
        return;
    }
    toks.eat();

    if (ssa->get_last_instr().type != InstrType::RET) {
        if (!isVoid)
            WARN("Missing explicit return\n");
        ssa->add_stack(-1);
        ssa->add_instr(InstrType::RET);
    }

    // swap back to main ssa
    ssa = &ssa_stack[0];
}

// funcBody = [ varDecl ] “{” [ statSequence ] “}”
void Parser::funcBody()
{
    // [ varDecl ]
    if (toks.get_type() == TokenType::VAR)
        varDecl();

    // "{"
    if (toks.get_type() != TokenType::LBRACE) {
        SYN_EXPECTED("LBRACE");
        return;
    }
    toks.eat();

    // make statSequence optional
    statSequence();

    // }
    if (toks.get_type() != TokenType::RBRACE) {
        SYN_EXPECTED("RBRACE");
        return;
    }
    toks.eat();
}

void Parser::statSequence()
{
    bool wasReturn = false;
    do {
        wasReturn = statement();
        if (toks.get_type() == TokenType::SEMI)
            toks.eat();
        else
            break;
    } while (!wasReturn);
}

// statement = assignment | funcCall | ifStatement | whileStatement | returnStatement
bool Parser::statement()
{
    switch (toks.get_type()) {
    case TokenType::LET:
        assignment();
        break;
    case TokenType::CALL:
        if (!funcCall()) {
            ERROR("Expected void function\n");
            exit(1);
        }
        ssa->clear_stack();
        break;
    case TokenType::IF:
        ifStatement();
        break;
    case TokenType::WHILE:
        whileStatement();
        break;
    case TokenType::RET:
        returnStatement();
        ssa->clear_stack();
        return true;
        break;
    default:
        break;
    }
    return false;
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

    ssa->set_symbol(tok);

    // add placeholder if current is empty
    if (ssa->get_current_block()->empty())
        ssa->add_instr(InstrType::NONE);
}

// funcCall = “call” ident [ “(“ [expression { “,” expression } ] “)” ]
bool Parser::funcCall()
{
    toks.eat(); // eat CALL

    if (toks.get_type() != TokenType::ID) {
        SYN_EXPECTED("ID");
    }
    auto val = toks.get();
    toks.eat(); // ID

    std::vector<u64> args;
    if (toks.get_type() == TokenType::LPAREN) {
        toks.eat(); // LPAREN
        do {
            expression();
            if (ssa->size_stack() > 0) {
                args.push_back(ssa->top_stack());
                ssa->pop_stack();
            }
            if (toks.get_type() == TokenType::COMMA) {
                toks.eat(); // COMMA
                if (toks.get_type() == TokenType::RPAREN) {
                    SYN_EXPECTED("Expression");
                    return false;
                }
            }
        } while (toks.get_type() != TokenType::RPAREN);
        toks.eat(); // RPAREN
    }

    // USER DEFINED FUNCTION
    if (functionMap.find(*val->val()) != functionMap.end()) {
        auto& [jmp_pos, paramCount, isVoid] = functionMap.at(*val->val());
        if (paramCount != args.size()) {
            SYN_ERROR("Expected %llu arguments but got %zu\n", paramCount, args.size());
            exit(1);
        }

        // set params
        u64 arg_num = 0;
        for (auto& e : args) {
            ssa->add_stack(++arg_num); // param num
            ssa->add_stack(e); // pos
            ssa->add_instr(InstrType::SETP);
        }

        // jmp
        ssa->add_stack(jmp_pos);
        ssa->add_instr(InstrType::JUMP);

        // only add last_pos to stack if non void as should not be used as expression if void
        if (!isVoid)
            ssa->add_stack(ssa->get_last_pos());

        INFO("Function (%s) is %s\n", val->id().c_str(), (isVoid ? "void" : "non-void"));
        return isVoid;
    } else { // inbuilt function
        for (auto it = args.rbegin(); it != args.rend(); it++) {
            ssa->add_stack(*it);
        }
        return ssa->resolve_symbol(val);
    }
}

// if = “if” relation “then” statSequence [ “else” statSequence ] “fi”
void Parser::ifStatement()
{
    toks.eat(); // IF
    auto dominator = ssa->get_current_block();

    relation();

    auto left = ssa->add_block(true);
    ssa->reverse_block();
    auto right = ssa->add_block(false);
    JoinNodeType join = {};
    join.node = std::make_shared<Block>();
    join.isLeft = std::nullopt;
    auto old_symbols = ssa->add_symbols_to_block(join);
    ssa->join_stack.emplace_back(std::move(join));

    left->dominator = dominator;
    right->dominator = dominator;
    ssa->join_stack.back().node->dominator = dominator;

    auto& [join_block, isBranchLeft, idToPhi, _] = ssa->join_stack.back();
    assert(left->parent_left == right->parent_left);
    auto parent = left->parent_left;

    if (toks.get_type() != TokenType::THEN) {
        SYN_EXPECTED("THEN");
        return;
    }
    toks.eat(); // then

    ssa->set_current_block(left);
    isBranchLeft = true;

    INFO("[%s] entering left statSequence\n", __func__);
    statSequence();
    INFO("[%s] exiting left statSequence\n", __func__);
    left = ssa->get_current_block();

    isBranchLeft = false;
    auto original_right = right;
    ssa->set_current_block(right);
    ssa->add_instr(InstrType::NONE);
    if (toks.get_type() == TokenType::ELSE) {
        toks.eat(); // else
        ssa->restore_symbol_state(old_symbols);
        INFO("[%s] entering right statSequence\n", __func__);
        statSequence();
        INFO("[%s] exiting right statSequence\n", __func__);
        right = ssa->get_current_block();
    } else {
        ssa->add_instr(InstrType::NONE);
    }

    if (toks.get_type() != TokenType::FI) {
        SYN_EXPECTED("FI");
        return;
    }
    toks.eat(); // FI

    isBranchLeft = std::nullopt;

    // join
    join_block->parent_left = left;
    join_block->parent_right = right;
    left->right = join_block;
    right->left = join_block;

    ssa->set_current_block(join_block);

    ssa->resolve_phi(idToPhi);
    ssa->commit_phi(idToPhi, true);

    // add branch if to left if join_block has instructions
    if (!join_block->empty()) {
        auto current = ssa->get_current_block();
        ssa->set_current_block(left);
        ssa->add_stack(join_block->front().pos);
        ssa->add_instr(InstrType::BRA);
        ssa->set_current_block(current);
    }

    ssa->resolve_branch(parent, original_right);

    ssa->join_stack.pop_back();
}

// while = “while” relation “do” StatSequence “od”
void Parser::whileStatement()
{
    toks.eat(); // while

    JoinNodeType join = {};
    join.isLeft = false;

    if (ssa->get_current_block()->size() == 1 && ssa->get_current_block()->back().type == InstrType::NONE) {
        join.node = ssa->get_current_block();
    } else {
        join.node = ssa->add_block(true);
    }

    auto exit_block = ssa->add_block(false);
    ssa->add_instr(InstrType::NONE);

    ssa->set_current_block(join.node);
    ssa->add_symbols_to_block(join);
    ssa->resolve_phi(join.idToPhi);
    relation();
    join.whileInfo = ssa->get_cmp();
    ssa->join_stack.emplace_back(std::move(join));
    auto& join_block = ssa->join_stack.back().node;
    auto loop = ssa->add_block(true);

    loop->dominator = ssa->join_stack.back().node;
    exit_block->dominator = ssa->join_stack.back().node;

    if (toks.get_type() != TokenType::DO) {
        SYN_EXPECTED("DO");
        return;
    }
    toks.eat();

    statSequence();
    auto end_block = ssa->get_current_block();
    end_block->entry = join_block;

    if (toks.get_type() != TokenType::OD) {
        SYN_EXPECTED("OD");
    } else
        toks.eat();
    ssa->set_current_block(exit_block);

    ssa->resolve_phi(ssa->join_stack.back().idToPhi);
    ssa->commit_phi(ssa->join_stack.back().idToPhi);

    auto curr = ssa->get_current_block();
    ssa->set_current_block(end_block);
    ssa->add_stack(join_block->front().pos);
    ssa->add_instr(InstrType::BRA);
    ssa->set_current_block(curr);

    ssa->resolve_branch(join_block, exit_block);

    ssa->join_stack.pop_back();
}

// return
void Parser::returnStatement()
{
    toks.eat(); // "return"

    auto prev = ssa->size_stack();
    expression();

    if (ssa->size_stack() <= prev)
        ssa->add_stack(-1);

    ssa->add_instr(InstrType::RET);
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
            ssa->add_instr(InstrType::ADD);
            break;
        case TokenType::MIN:
            toks.eat();
            term();
            ssa->add_instr(InstrType::SUB);
            break;
        default:
            loop = false;
            break;
        }
        // TODO: might not need
        // if (loop)
        //     ssa->add_stack(ssa->get_last_pos());
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
            ssa->add_instr(InstrType::MUL);
            break;
        case TokenType::DIV:
            toks.eat();
            factor();
            ssa->add_instr(InstrType::DIV);
            break;
        default:
            loop = false;
            break;
        }
        // TODO: might not need
        // if (loop)
        //     ssa->add_stack(ssa->get_last_pos());
    } while (loop);
}

// factor = ident | number | “(“ expression “)” | funcCall
void Parser::factor()
{
    switch (toks.get_type()) {
    case TokenType::ID:
        ssa->resolve_symbol(toks.get());
        toks.eat();
        break;
    case TokenType::NUM:
        ssa->add_const(*toks.get()->val());
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
        if (funcCall()) {
            ERROR("Expected non-void function\n");
            exit(1);
        }
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
        ssa->add_instr(InstrType::BNE);
    } break;
    case TokenType::NEQ:
        toks.eat();
        expression();
        ssa->add_instr(InstrType::BEQ);
        break;
    case TokenType::LT:
        toks.eat();
        expression();
        ssa->add_instr(InstrType::BGE);
        break;
    case TokenType::LTEQ:
        toks.eat();
        expression();
        ssa->add_instr(InstrType::BGT);
        break;
    case TokenType::GT:
        toks.eat();
        expression();
        ssa->add_instr(InstrType::BLE);
        break;
    case TokenType::GTEQ:
        toks.eat();
        expression();
        ssa->add_instr(InstrType::BLT);
        break;
    default:
        SYN_EXPECTED("Relation");
        break;
    }
}
