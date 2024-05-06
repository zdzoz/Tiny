#include "ssa.h"

#include <iostream>
#include <ostream>

u64 Block::__id = 0;
u64 SSA::instruction_num = 0;

SSA::SSA()
    : blocks(std::make_shared<Block>())
    , current(blocks)
{
    add_block(true);
}

void SSA::add_block(bool isLeft)
{
    auto p = current;
    current = std::make_shared<Block>();
    current->parent = p;
    if (isLeft) {
        p->left = current;
        INFO("Created BB%llu\n", blocks->left->block_id);
    } else {
        p->right = current;
        INFO("Created BB%llu\n", blocks->right->block_id);
    }
}

void SSA::add_const(u64 val)
{
    static std::unordered_map<u64, u64> val_to_pos;
    if (val_to_pos.find(val) != val_to_pos.end()) {
        // last_instr_pos = val_to_pos[val];
        options.push(val_to_pos[val]);
        return;
    }

    Instr instr;
    instr.type = InstrType::CONST;
    instr.x = val;
    instr.pos = instruction_num++;
    last_instr_pos = instr.pos;
    val_to_pos[val] = instr.pos;

    blocks->add(std::move(instr));
    options.push(val);
}

// FIX: add_instr
void SSA::add_instr(InstrType type)
{
    Instr instr;
    instr.type = type;
    switch (type) {
    case InstrType::ADD: // add x y addition
    case InstrType::SUB: // sub x y subtraction
    case InstrType::MUL: // mul x y multiplication
    case InstrType::DIV: // div x y division
    case InstrType::CMP: // cmp x y comparison
    case InstrType::BNE: // bne x y branch to y on x not equal
    case InstrType::BEQ: // beq x y branch to y on x equal
    case InstrType::BLE: // ble x y branch to y on x less or equal
    case InstrType::BLT: // blt x y branch to y on x less
    case InstrType::BGE: // bge x y branch to y on x greater or equal
    case InstrType::BGT: { // bgt x y branch to y on x greater
        assert(options.size() >= 2 && "Not enough options");
        instr.y = options.top();
        options.pop();
        instr.x = options.top();
        options.pop();
    } break;
    case InstrType::READ: // no opts
        break;
    case InstrType::WRITE: {
        instr.x = options.top();
        options.pop();
    } break;
    case InstrType::WRITENL: // no opts
        break;
    case InstrType::BRA: // bra y branch to y
    case InstrType::PHI: // phi x1 x2 compute Phi(x1,    x2)
    default:
        ERROR("Unknown InstrType\n");
        exit(1);
    }
    add_to_block(std::move(instr));
}

void SSA::add_to_block(Instr&& instr)
{
    instr.pos = instruction_num++;
    last_instr_pos = instr.pos;
    current->add(std::move(instr));
}

void SSA::add_symbols(u64 count)
{
    symbol_table.resize(symbol_table.size() + count);
    INFO("symbol_table size %zu\n", symbol_table.size());
}

void SSA::set_symbol(const Token* t, u64 pos)
{
    if (*t->val() >= symbol_table.size()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }

    INFO("Setting %s = %llu\n", t->id().c_str(), pos);
    symbol_table[*t->val()] = pos;
}

// void SSA::resolve_symbol(const Token* t)
// {
//     if (*t->val() >= symbol_table.size()) {
//         LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
//         exit(1);
//     }
//
//     switch (symbol_table[*t->val()].value()) {
//     case FUNC_INPUT_NUM:
//         add_instr(InstrType::READ);
//         return;
//     case FUNC_OUTPUT_NUM:
//         add_instr(InstrType::WRITE);
//         return;
//     case FUNC_OUTPUT_NL:
//         add_instr(InstrType::WRITE);
//         return;
//     default:
//         TODO("FIX: user defined functions (%s)\n", __func__);
//         exit(1);
//     }
// }

// resolves symbol and adds to option stack
bool SSA::resolve_symbol(const Token* t)
{
    if (*t->val() >= symbol_table.size()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }

    u64 opt = 0;
    switch (*t->val()) {
    case FUNC_INPUT_NUM: // no opt
        add_instr(InstrType::READ);
        return false;
    case FUNC_OUTPUT_NUM: // has opt
        add_instr(InstrType::WRITE);
        return false;
    case FUNC_OUTPUT_NL: // no opt
        add_instr(InstrType::WRITENL);
        return false;
    default:
        if (symbol_table[*t->val()].has_value()) {
        } else {
            add_const(0);
            symbol_table[*t->val()] = 0;
            WARN("uninitialized symbol '%s'\n", t->id().c_str());
        }
        opt = *symbol_table[*t->val()];
    }
    options.push(opt);
    return true;
}

/// PRINTS

void SSA::print_symbol_table()
{
    std::cerr << "Symbol Table:" << std::endl;
    u64 i = 0;
    for (const auto& e : symbol_table) {
        if (e)
            std::cerr << "\t" << i << ": " << *e << std::endl;
        else
            std::cerr << "\t" << i << ": Unset" << std::endl;
        i++;
    }
    std::cerr << std::endl;
}

// TODO: check if print works correctly
std::ostream& operator<<(std::ostream& os, const SSA& ssa)
{
    // └┐├─│
    std::function<void(Block*, u32)> p_blocks = [&os, &p_blocks](const Block* const block, u32 indent) -> void {
        os << std::string(indent, ' ') << (indent ? "├ " : "") << "BB" << block->get_block_id() << std::endl;
        block->print_with_indent(os, indent);

        if (block->right || block->left)
            os << "└───────────┐" << std::endl;
        if (block->left)
            p_blocks(block->left.get(), indent + 12);
        if (block->right)
            p_blocks(block->right.get(), indent + 12);
    };
    p_blocks(ssa.blocks.get(), 0);

    return os;
}

void Block::print_with_indent(std::ostream& os, u64 indent) const
{
    if (indent == 0) {
        for (const auto& instr : instructions) {
            os << instr << std::endl;
        }
        return;
    }
    for (const auto& instr : instructions) {
        os << std::string(indent, ' ') << "│" << ' ' << instr << std::endl;
    }
}

std::ostream& operator<<(std::ostream& os, const Block& b)
{
    b.print_with_indent(os, 0);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Instr& instr)
{
    const auto common = [&]() {
        os << " (" << instr.x << ") (" << instr.y << ")";
    };

    os << instr.pos << ": ";
    switch (instr.type) {
    case InstrType::ADD:
        os << "add";
        common();
        break;
    case InstrType::SUB:
        os << "sub";
        common();
        break;
    case InstrType::MUL:
        os << "mul";
        common();
        break;
    case InstrType::DIV:
        os << "div";
        common();
        break;
    case InstrType::CMP:
        os << "cmp";
        common();
        break;
    case InstrType::PHI:
        os << "phi";
        common();
        break;
    case InstrType::END:
        os << "end";
        common();
        break;
    case InstrType::BNE:
        os << "bne";
        common();
        break;
    case InstrType::BEQ:
        os << "beq";
        common();
        break;
    case InstrType::BLE:
        os << "ble";
        common();
        break;
    case InstrType::BLT:
        os << "blt";
        common();
        break;
    case InstrType::BGE:
        os << "bge";
        common();
        break;
    case InstrType::BGT:
        os << "bgt";
        common();
        break;
    case InstrType::BRA:
        os << "bra " << instr.y;
        break;
    case InstrType::CONST:
        os << "const #" << instr.x;
        break;
    case InstrType::WRITE:
        os << "write " << instr.x;
        break;
    case InstrType::WRITENL:
        os << "writeNL";
        break;
    case InstrType::READ:
        os << "read";
        break;
    default:
        break;
    }
    return os;
}
