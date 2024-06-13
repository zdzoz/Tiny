#include "ssa.h"
#include "token.h"

#include <algorithm>
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
    if (constants.find(val) != constants.end()) {
        last_instr_pos = constants[val];
        add_stack(constants[val]);
        return;
    }

    Instr instr;
    instr.type = InstrType::CONST;
    instr.x = val;
    instr.pos = instruction_num++;
    last_instr_pos = instr.pos;
    constants[val] = instr.pos;

    blocks->add_back(std::move(instr));
    add_stack(constants[val]);
}

static bool isBranch(InstrType type)
{
    switch (type) {
    case InstrType::BNE:
    case InstrType::BEQ:
    case InstrType::BLE:
    case InstrType::BLT:
    case InstrType::BGT:
    case InstrType::BGE:
        return true;
    default:
        break;
    }
    return false;
}

void SSA::add_instr(InstrType type)
{
    Instr __instr;
    Instr* instr = &__instr;
    bool none = false;
    bool add_to_stack = false;
    if (!isBranch(type) && !get_current_block()->empty() && get_current_block()->back().type == InstrType::NONE) {
        instr = &get_current_block()->back();
        none = true;
    }

    instr->type = type;
    switch (type) {
    case InstrType::BNE: // bne x y branch to y on x not equal
    case InstrType::BEQ: // beq x y branch to y on x equal
    case InstrType::BLE: // ble x y branch to y on x less or equal
    case InstrType::BLT: // blt x y branch to y on x less
    case InstrType::BGT: // bgt x y branch to y on x greater
    case InstrType::BGE: { // bge x y branch to y on x greater or equal
        add_instr(InstrType::CMP);
        assert(instr_stack.size() == 1);
        instr->x = instr_stack.top();
        instr_stack.pop();
    } break;
    case InstrType::CMP: // cmp x y comparison
    case InstrType::ADD: // add x y addition
    case InstrType::SUB: // sub x y subtraction
    case InstrType::MUL: // mul x y multiplication
    case InstrType::DIV: { // div x y division
        assert(instr_stack.size() >= 2 && "Not enough options");
        instr->y = instr_stack.top();
        instr_stack.pop();
        instr->x = instr_stack.top();
        instr_stack.pop();
        add_to_stack = true;
    } break;
    case InstrType::READ: // no opts
        add_to_stack = true;
        break;
    case InstrType::WRITE: {
        assert(instr_stack.size() >= 1 && "Not enough options");
        instr->x = instr_stack.top();
        instr_stack.pop();
    } break;
    case InstrType::WRITENL: // no opts
        break;
    case InstrType::SETP:
        assert(instr_stack.size() >= 2 && "Not enough options");
        instr->x = instr_stack.top(); // val
        instr_stack.pop();
        instr->y = instr_stack.top(); // specifies which param
        instr_stack.pop();
        break;
    case InstrType::JUMP:
    case InstrType::RET:
        assert(instr_stack.size() >= 1 && "Not enough options");
        instr->x = instr_stack.top();
        instr_stack.pop();
        break;
    case InstrType::GETP:
        assert(instr_stack.size() >= 1 && "Not enough options");
        instr->y = instr_stack.top(); // y is used to specify which param
        instr_stack.pop();
        break;
    case InstrType::NONE:
        break;
    case InstrType::BRA: { // bra y branch to y
        assert(instr_stack.size() == 1 && "Not enough options");
        instr->y = instr_stack.top();
        instr_stack.pop();
    } break;
    case InstrType::PHI: // phi x1 x2 compute Phi(x1,    x2)
        ERROR("PHI should not be created directly in %s\n", __func__);
        break;
    default:
        ERROR("Unknown InstrType\n");
        exit(1);
    }

    if (instr->isHashable() && expressions.find(*instr) != expressions.end()) {
        last_instr_pos = expressions[*instr];
        if (add_to_stack)
            add_stack(get_last_pos());
        instr->type = InstrType::NONE;
        return;
    }

    if (!none)
        add_to_block(__instr);
    else {
        last_instr_pos = instr->pos;
    }

    if (instr->isHashable())
        expressions[*instr] = get_last_pos();

    if (add_to_stack)
        add_stack(get_last_pos());
}

void SSA::add_to_block(Instr instr)
{
    instr.pos = instruction_num++;
    last_instr_pos = instr.pos;
    current->add_back(std::move(instr));
}

// NOTE: adds to front of stack, useful for phi
Instr& SSA::add_to_block(Instr&& instr, std::shared_ptr<Block>& join_block)
{
    instr.pos = instruction_num++;
    // last_instr_pos = instr.pos; NOTE: idk if i need this
    return join_block->add_front(std::move(instr));
}

void SSA::add_symbols(SymbolTableType&& v)
{
    symbol_table.insert(v.begin(), v.end());
    INFO("symbol_table size %zu\n", symbol_table.size());
}

void SSA::set_symbol(const Token* t)
{
    if (symbol_table.find(*t->val()) == symbol_table.end()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }
    u64 pos = instr_stack.top();
    instr_stack.pop();

    if (!join_stack.empty() && join_stack.back().isLeft.has_value()) {
        auto& [join_block, isBranchLeft, idToPhi, whileInfo] = join_stack.back();
        Instr* phi;
        if (idToPhi.find(*t->val()) != idToPhi.end()) {
            phi = idToPhi[*t->val()];
        } else {
            // NOTE: should never reach
            PRTOKLN(*t);
            print_symbol_table();
            std::cout << *this;
            throw std::runtime_error("unreachable " + std::string(__func__));
        }
        if (*isBranchLeft)
            phi->x = pos;
        else {
            if (!phi->x.has_value()) {
                phi->x = pos;
            }
            phi->y = pos;
        }
    }
    INFO("Setting %s = %llu\n", t->id().c_str(), pos);
    symbol_table[*t->val()].second = pos;
}

std::vector<std::pair<u64, u64>> SSA::add_symbols_to_block(JoinNodeType& join_node)
{
    std::vector<std::pair<u64, u64>> old_symbols;
    for (auto& [k, s] : symbol_table) {
        if (k < inbuilt_count)
            continue;

        if (s.second.has_value())
            old_symbols.emplace_back(std::make_pair(k, *s.second));
        Instr p = {};
        p.type = InstrType::PHI;
        p.x = s.second;
        p.y = p.x;
        join_node.idToPhi[k] = &add_to_block(std::move(p), join_node.node);
    }
    return old_symbols;
}

// resolves symbol and adds to option stack
// for functions returns true if isVoid
bool SSA::resolve_symbol(const Token* t)
{
    if (symbol_table.find(*t->val()) == symbol_table.end()) {
        LOG_ERROR("[ERROR] Unknown symbol '%s'\n", t->id().c_str());
        exit(1);
    }

    u64 opt = 0;
    switch (*t->val()) {
    case FUNC_INPUT_NUM: // no opt
        add_instr(InstrType::READ);
        return false; // is not void
    case FUNC_OUTPUT_NUM: // has opt
        add_instr(InstrType::WRITE);
        return true;
    case FUNC_OUTPUT_NL: // no opt
        add_instr(InstrType::WRITENL);
        return true;
    default:
        auto& [_, val] = symbol_table[*t->val()];
        if (val.has_value()) {
            opt = val.value();
        } else {
            WARN("uninitialized symbol '%s'\n", t->id().c_str());
            add_const(0);
            val = instr_stack.top();
            opt = val.value();
            pop_stack();
        }
    }
    add_stack(opt);
    last_instr_pos = opt;
    return true;
}

void SSA::restore_symbol_state(std::vector<std::pair<u64, u64>>& old_symbols)
{
    for (auto& [k, v] : old_symbols) {
        if (symbol_table.find(k) == symbol_table.end()) {
            throw std::runtime_error("Failed to find symbol");
        }
        symbol_table[k].second = v;
    }
}

void SSA::resolve_branch(std::shared_ptr<Block>& from, std::shared_ptr<Block>& to)
{
    assert(to->instructions.size() != 0);
    auto& instr_pos = to->instructions.front().pos;
    auto& p = from->instructions;
    p.back().y = instr_pos;
}

std::vector<std::pair<u64, u64>> SSA::resolve_phi(std::unordered_map<u64, Instr*>& idToPhi)
{
    std::vector<std::pair<u64, u64>> old_symbols;
    for (auto& [id, phi] : idToPhi) {
        INFO("Resolving phi of %s = %llu\n", symbol_table[id].first.c_str(), phi->pos);
        symbol_table[id].second = phi->pos;
        old_symbols.emplace_back(id, phi->pos);
    }
    return old_symbols;
}

// if phi.x == phi.y should resolve to phi.x and phi.x should be propogated until left and right nullptr update outer too
void SSA::commit_phi(std::unordered_map<u64, Instr*>& idToPhi, bool isIf)
{
    std::vector<u64> erase_queue;
    std::vector<u64> erase_queue2;

    std::function<void(Block*, u64, u64)> propagate_changes = [&](Block* start, u64 from, u64 to) {
        u64 idx = 0;
        std::vector<u64> eq;
        for (auto it = start->instructions.begin(); it != start->instructions.end(); ++it) {
            auto& instr = *it;
            if (instr.pos == from)
                continue;
            if (instr.x == from || instr.y == from) {
                if (instr.x == from) {
                    instr.x = to;
                }
                if (instr.y == from) {
                    instr.y = to;
                }
                if (instr.isHashable() && expressions.find(instr) != expressions.end()) {
                    propagate_changes(blocks.get(), instr.pos, expressions[instr]);
                    eq.push_back(idx);
                }
            }
            idx++;
        }

        std::sort(eq.begin(), eq.end(), std::greater<u64>());
        for (auto& e : eq)
            start->instructions.erase(start->instructions.begin() + e);

        if (start->left) {
            propagate_changes(start->left.get(), from, to);
        }
        if (start->right) {
            propagate_changes(start->right.get(), from, to);
        }
    };

    for (auto& [id, phi] : idToPhi) {
        // check for cyclic reference
        if (phi->pos == phi->y) {
            phi->y = phi->x;
        } else if (phi->pos == phi->x) {
            phi->x = phi->y;
        }

        if (phi->x == phi->y) {
            auto& instrs = join_stack.back().node->instructions;
            if (phi->x.has_value()) {
                propagate_changes(join_stack.back().node.get(), phi->pos, phi->x.value());
                symbol_table[id].second = phi->x; // update symbol table
            }

            // remove instruction from block
            if (!isIf)
                erase_queue.emplace_back(id);
            u64 it = 0;
            for (auto& instr : instrs) {
                if (phi->pos == instr.pos) {
                    INFO("ERASING %s == %llu\n", symbol_table[id].first.c_str(), phi->pos);
                    erase_queue2.push_back(it);
                    break;
                }
                it++;
            }
        }
    }

    for (auto& e : erase_queue)
        idToPhi.erase(e);

    if (join_stack.size() > 1) {
        auto& outer = join_stack[join_stack.size() - 2];
        for (auto& i : idToPhi) {
            if (outer.idToPhi.find(i.first) != outer.idToPhi.end()) {
                if (outer.idToPhi[i.first]->y)
                    INFO("Setting %llu to %llu\n", outer.idToPhi[i.first]->y.value(), i.second->pos);
                auto& toUpdate = outer.idToPhi[i.first];
                auto updatedVal = i.second->pos;
                if (isIf && i.second->x == i.second->y) {
                    updatedVal = i.second->x.value_or(updatedVal);
                }
                if (toUpdate->x == toUpdate->y) {
                    toUpdate->x = updatedVal;
                } else {
                    toUpdate->y = updatedVal;
                }
            }
        }
    }

    std::sort(erase_queue2.begin(), erase_queue2.end(), std::greater<u64>()); // sort descending

    auto& back = join_stack.back().node->instructions;
    for (auto& e : erase_queue2)
        back.erase(back.begin() + e);

    if (join_stack.back().node->instructions.empty()) {
        auto current = get_current_block();
        set_current_block(join_stack.back().node);
        add_instr(InstrType::NONE);
        set_current_block(current);
    }
    assert(!join_stack.back().node->instructions.empty());
}

/// DOT GENERATION ///

// Record:      bb0 [shape=record, label="<b>BB0 | {3: const #0}"]
// Link:        bb0:s -> bb1:n
// Dominator:   bb1:b -> bb2:b [color=blue, style=dotted, label="dom"]
// block = b
// instr_top = n
// instr_bot = s
void SSA::generate_dot() const
{
    using namespace std::string_literals;

    constexpr auto record_start = [](u64 block_id) {
        auto id = std::to_string(block_id);
        return "bb" + id + "[shape=record, label=\"<b>BB" + id + "| {";
    };
    constexpr auto record_end = []() { return "}\"]"; };

    constexpr auto create_link = [](u64 pid, u64 cid, const std::string& label) {
        return "bb"s + std::to_string(pid) + ":s -> bb" + std::to_string(cid) + ":n" + "[label=\"" + label + "\"]";
    };

    constexpr auto create_dominator = [](u64 dominator, u64 dominated) {
        return "bb" + std::to_string(dominator) + ":b -> bb" + std::to_string(dominated) + ":b [color=blue, style=dotted, label=\"dom\"]";
    };

    std::unordered_set<u64>
        seen = {};
    std::function<void(Block*)> p_blocks = [&](const Block* const block) -> void {
        if (seen.find(block->get_block_id()) != seen.end()) {
            return;
        }
        seen.insert(block->get_block_id());

        std::cout << record_start(block->block_id);
        for (auto& instr : block->instructions) {
            std::cout << instr;
            if (&instr != &block->instructions.back())
                std::cout << "|";
        }
        std::cout << record_end() << std::endl;

        std::string primary_str = "";
        std::string secondary_str = "fall";
        if (block->parent_left && block->parent_right) {
            primary_str = "branch";
        }
        // parent_right is unused
        if (block->parent_left && block->parent_left->left && block->parent_left->right) {
            primary_str = block->parent_left->right.get() == block ? "branch" : "fall";
        }
        if (block->parent_left) {
            std::cout << create_link(block->parent_left->block_id, block->block_id, primary_str) << std::endl;
        }
        if (block->parent_right) {
            std::cout << create_link(block->parent_right->block_id, block->block_id, secondary_str) << std::endl;
        }
        if (block->dominator)
            std::cout << create_dominator(block->dominator->block_id, block->block_id) << std::endl;
        if (block->entry)
            std::cout << create_link(block->block_id, block->entry->block_id, "branch") << std::endl;

        if (block->left)
            p_blocks(block->left.get());
        if (block->right)
            p_blocks(block->right.get());
    };
    std::cout << "digraph " << this->name << " {\n";
    p_blocks(this->blocks.get());
    std::cout << "}\n";
}

/// PRINTS

void SSA::print_symbol_table()
{
    std::cerr << "Instruction Stack Size: " << instr_stack.size() << std::endl;
    std::cerr << "Inbuilt symbols: " << inbuilt_count << std::endl;
    std::cerr << "Total symbols: " << symbol_table.size() << std::endl;
    std::cerr << "Symbol Table (" << this->name << "):" << std::endl;
    for (const auto& [key, val] : symbol_table) {
        std::cerr << "\t[" << key << "] " << std::left << std::setw(10) << val.first << " ";
        if (val.second)
            std::cerr << *val.second << std::endl;
        else
            std::cerr << "Unset" << std::endl;
    }
    std::cerr << std::endl;
}

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
        os << " (" << *instr.x << ") (" << *instr.y << ")";
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
        os << "bra " << *instr.y;
        break;
    case InstrType::CONST:
        os << "const #" << *instr.x;
        break;
    case InstrType::WRITE:
        os << "write " << *instr.x;
        break;
    case InstrType::WRITENL:
        os << "writeNL";
        break;
    case InstrType::READ:
        os << "read";
        break;
    case InstrType::GETP:
        os << "getpar" << *instr.y;
        break;
    case InstrType::SETP:
        os << "setpar" << *instr.y << " " << *instr.x;
        break;
    case InstrType::JUMP:
        os << "jsr " << *instr.x;
        break;
    case InstrType::RET:
        os << "ret " << (int64_t)*instr.x;
        break;
    case InstrType::NONE:
        os << "none";
        break;
    default:
        break;
    }
    return os;
}
