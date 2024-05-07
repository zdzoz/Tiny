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

std::shared_ptr<Block> SSA::reverse_block()
{
    auto t = current;
    current = current->parent_left;
    return t;
}

std::shared_ptr<Block> SSA::add_block(bool isLeft)
{
    auto p = current;
    current = std::make_shared<Block>();
    current->parent_left = p;
    if (isLeft) {
        p->left = current;
        INFO("Created BB%llu\n", current->block_id);
    } else {
        p->right = current;
        INFO("Created BB%llu\n", current->block_id);
    }
    return current;
}

void SSA::add_const(u64 val)
{
    static std::unordered_map<u64, u64> val_to_pos;
    if (val_to_pos.find(val) != val_to_pos.end()) {
        // last_instr_pos = val_to_pos[val];
        add_stack(val_to_pos[val]);
        return;
    }

    Instr instr;
    instr.type = InstrType::CONST;
    instr.x = val;
    instr.pos = instruction_num++;
    last_instr_pos = instr.pos;
    val_to_pos[val] = instr.pos;

    blocks->add(std::move(instr));
    add_stack(val_to_pos[val]);
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
        assert(instr_stack.size() >= 2 && "Not enough options");
        instr.y = instr_stack.top();
        instr_stack.pop();
        instr.x = instr_stack.top();
        instr_stack.pop();
    } break;
    case InstrType::READ: // no opts
        break;
    case InstrType::WRITE: {
        instr.x = instr_stack.top();
        instr_stack.pop();
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

void SSA::add_symbols(SymbolTableType&& v)
{
    symbol_table.reserve(symbol_table.size() + v.size());
    symbol_table.insert(symbol_table.end(), v.begin(), v.end());
    INFO("symbol_table size %zu\n", symbol_table.size());

    // auto it = symbol_table.begin() + v.size();
    // for (auto& e : it) {
    // }
}

void SSA::set_symbol(const Token* t)
{
    if (*t->val() >= symbol_table.size()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }
    u64 pos = instr_stack.top();
    instr_stack.pop();

    INFO("Setting %s = %llu\n", t->id().c_str(), pos);
    // symbol_table[*t->val()] = pos;
    symbol_table[*t->val()] = std::make_tuple(t->id(), pos);
}

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
        add_stack(get_last_pos());
        return false;
    case FUNC_OUTPUT_NUM: // has opt
        add_instr(InstrType::WRITE);
        add_stack(get_last_pos());
        return false;
    case FUNC_OUTPUT_NL: // no opt
        add_instr(InstrType::WRITENL);
        add_stack(get_last_pos());
        return false;
    default:
        // if (symbol_table[*t->val()].has_value()) {
        const auto& [_, val] = symbol_table[*t->val()];
        if (val.has_value()) {
            opt = *std::get<1>(symbol_table[*t->val()]);
        } else {
            add_const(0);
            auto& [_, sval] = symbol_table[*t->val()];
            sval = 0;
            // symbol_table[*t->val()] = std::make_tuple(t->id(), 0);
            WARN("uninitialized symbol '%s'\n", t->id().c_str());
            opt = 0;
        }
    }
    add_stack(opt);
    return true;
}

/// PRINTS

void SSA::print_symbol_table()
{
    std::cerr << "Instruction Stack Size: " << instr_stack.size() << std::endl;
    std::cerr << "Symbol Table:" << std::endl;
    u64 i = 0;
    for (const auto& [symbol, e] : symbol_table) {
        std::cerr << "\t[" << i << "] " << std::left << std::setw(10) << symbol << " ";
        if (e)
            std::cerr << *e << std::endl;
        else
            std::cerr << "Unset" << std::endl;
        i++;
    }
    std::cerr << std::endl;
}

// TODO: check if print works correctly
std::ostream& operator<<(std::ostream& os, const SSA& ssa)
{
    std::unordered_set<u64> seen = {};
    // └┐├─│
    std::function<void(Block*, u32)> p_blocks = [&](const Block* const block, u32 indent) -> void {
        if (seen.find(block->get_block_id()) != seen.end()) {
            os << std::string(indent, ' ') << "└ BB" << block->get_block_id() << " merged" << std::endl;
            return;
        }
        seen.insert(block->get_block_id());
        os << std::string(indent, ' ') << (indent ? "├ " : "") << "BB" << block->get_block_id() << std::endl;
        block->print_with_indent(os, indent);

        if (block->right || block->left)
            os << std::string(indent, ' ') << "└───────────────┐" << std::endl;
        if (block->left)
            p_blocks(block->left.get(), indent + 16);
        if (block->right)
            p_blocks(block->right.get(), indent + 16);
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
