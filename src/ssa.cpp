#include "ssa.h"

#include <ostream>

u64 Block::__id = 0;
u64 SSA::instruction_num = 0;

void SSA::add_const(u64 val)
{
    static std::unordered_set<u64> consts;
    if (consts.find(val) != consts.end())
        return;

    consts.insert(val);

    Instr instr;
    instr.type = InstrType::CONST;
    instr.x = val;
    instr.id = instruction_num++;
    blocks->add(std::move(instr));
}

void SSA::add_instr(InstrType type)
{
    Instr instr;
    instr.type = type;
    instructions.emplace(std::move(instr));
}

Instr& SSA::get_curr()
{
    return instructions.top();
}

void SSA::add_symbols(const std::vector<u64>& s)
{
    symbol_table.reserve(symbol_table.size() + s.size());
    symbol_table.insert(symbol_table.end(), s.begin(), s.end());
    INFO("symbol_table size %zu\n", symbol_table.size());
}

// FIX: set_symbol
void SSA::set_symbol(Token* t)
{
    if (*t->val() > symbol_table.size()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }

    symbol_table[*t->val()] = 0;
}

u64 SSA::get_symbol(Token* t)
{
    if (*t->val() > symbol_table.size()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }

    return symbol_table[*t->val()].value_or([&]() {
        WARN("Uninitialized symbol '%s'", t->id().c_str());
        return 0;
    }());
}

/// PRINTS

// TODO: write print
std::ostream& operator<<(std::ostream& os, const SSA& ssa)
{
    TODO("Implement SSA %s\n", __func__);
    const auto& b = ssa.blocks;

    u32 indent = 0;
    os << "BB" << b->get_block_id() << std::endl;
    b->print_with_indent(os, indent);
    // "\n └──┐\n";

    // horizontal print?
    // void printBT(const std::string& prefix, const BSTNode* node, bool isLeft)
    // {
    //     if( node != nullptr )
    //     {
    //         std::cout << prefix;
    //
    //         std::cout << (isLeft ? "├──" : "└──" );
    //
    //         // print the value of the node
    //         std::cout << node->m_val << std::endl;
    //
    //         // enter the next tree level - left and right branch
    //         printBT( prefix + (isLeft ? "│   " : "    "), node->m_left, true);
    //         printBT( prefix + (isLeft ? "│   " : "    "), node->m_right, false);
    //     }
    // }
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

    os << instr.id << ": ";
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
    default:
        break;
    }
    return os;
}
