#pragma once

#include <iostream>

#include "token.h"

// InputNum, OutputNum, OutputNewLine
#define FUNC_INPUT_NUM 0
#define FUNC_OUTPUT_NUM 1
#define FUNC_OUTPUT_NL 2

typedef std::pair<std::string, std::optional<u64>> SymbolType;
// typedef std::vector<SymbolType> SymbolTableType;
typedef std::map<u64, SymbolType> SymbolTableType;

// token id -> jump pos, param count
struct FunctionType {
    u64 pos;
    u64 paramCount;
    bool isVoid; // true -> int, otherwise void
};

typedef std::unordered_map<u64, FunctionType> FunctionMap;

enum class InstrType {
    CONST, // const #x define an SSA value for a constant
    ADD, // add x y addition
    SUB, // sub x y subtraction
    MUL, // mul x y multiplication
    DIV, // div x y division
    CMP, // cmp x y comparison
    PHI, // phi x1 x2 compute Phi(x1,    x2)
    BRA, // bra y branch to y
    BNE, // bne x y branch to y on x not equal
    BEQ, // beq x y branch to y on x equal
    BLE, // ble x y branch to y on x less or equal
    BLT, // blt x y branch to y on x less
    BGE, // bge x y branch to y on x greater or equal
    BGT, // bgt x y branch to y on x greater

    // Inbuilt Functions
    READ,
    WRITE,
    WRITENL,

    // Functions
    JUMP,
    RET,
    SETP,
    GETP,

    END,
    NONE,
    UNKNOWN,
};

struct Instr {
    u64 pos;
    InstrType type;
    std::optional<u64> x, y;
    bool operator==(const Instr& other) const
    {
        if (type != other.type) return false;

        if (x == other.x && y == other.y) return true;
        if (x == other.y && y == other.x) return true;
        return false;
    }

    inline bool isHashable() const
    {
        switch (this->type) {
        case InstrType::ADD:
        case InstrType::SUB:
        case InstrType::MUL:
        case InstrType::DIV:
            return true;
        case InstrType::JUMP:
        case InstrType::SETP:
        case InstrType::GETP:
        case InstrType::RET:
        case InstrType::PHI:
        case InstrType::CMP:
        case InstrType::BRA:
        case InstrType::BNE:
        case InstrType::BEQ:
        case InstrType::BLE:
        case InstrType::BLT:
        case InstrType::BGE:
        case InstrType::BGT:
        case InstrType::READ:
        case InstrType::WRITE:
        case InstrType::WRITENL:
        case InstrType::END:
        case InstrType::CONST:
        case InstrType::NONE:
        case InstrType::UNKNOWN:
            break;
        }
        return false;
    }

    friend std::ostream& operator<<(std::ostream& os, const Instr& instr);
};

template <>
struct std::hash<Instr> {
    std::size_t operator()(const Instr& instr) const
    {
        using std::hash;
        using std::size_t;
        using std::string;

        assert(instr.isHashable());

        return ((hash<int>()((int)instr.type) >> 1) ^ (hash<std::optional<u64>>()(instr.x) ^ hash<std::optional<u64>>()(instr.y)));
    }
};

class SSA;
class Block {
    static u64 __id;
    u64 block_id;

    friend SSA;

public:
    // NOTE: maybe add sibling property?
    std::shared_ptr<Block> left, right, parent_left, parent_right, dominator, entry;

    Block()
        : block_id(__id++)
    {
    }

    inline u64 get_block_id() const { return block_id; }

    inline Instr& add_back(Instr&& instr)
    {
        instructions.emplace_back(instr);
        // return instructions.size() - 1;
        return instructions.back();
    }

    inline Instr& add_front(Instr&& instr)
    {
        instructions.emplace_front(instr);
        return instructions.front();
    }

    void print_with_indent(std::ostream& os, u64 indent) const;

    inline bool empty() { return instructions.empty(); }
    inline size_t size() { return instructions.size(); }
    inline Instr& front() { return instructions.front(); }
    inline Instr& back() { return instructions.back(); }
    inline void pop_back() { instructions.pop_back(); }

private:
    std::deque<Instr> instructions;

    // NOTE: std::deque instead of vector (allows references to items)
    friend std::ostream& operator<<(std::ostream& os, const Block& b);
};

typedef struct {
    std::shared_ptr<Block> node;
    std::optional<bool> isLeft;
    std::unordered_map<u64, Instr*> idToPhi;
    std::optional<Instr*> whileInfo;
} JoinNodeType;

class SSA {
public:
    SSA();

#ifndef NDEBUG
    ~SSA()
    {
        print_symbol_table();
    }
#endif

    std::shared_ptr<Block> reverse_block();
    std::shared_ptr<Block> add_block(bool isLeft);
    inline void set_current_block(std::shared_ptr<Block> b) { current = b; }
    inline std::shared_ptr<Block> get_current_block() { return current; }

    // gets first instr of current block
    inline u64 get_first_instr() const
    {
        assert(!current->empty());
        return current->front().pos;
    }

    inline const Instr& get_last_instr() const
    {
        assert(!current->empty());
        return current->back();
    }

    void add_const(u64 val);
    void add_instr(InstrType type);

    // void add_symbols(u64 count);
    void add_symbols(SymbolTableType&& v);
    void set_symbol(const Token* t);
    std::vector<std::pair<u64, u64>> add_symbols_to_block(JoinNodeType& join_node);

    bool resolve_symbol(const Token* t);
    void restore_symbol_state(std::vector<std::pair<u64, u64>>& old_symbols);
    void print_symbol_table();

    void resolve_branch(std::shared_ptr<Block>& from, std::shared_ptr<Block>& to);

    std::vector<std::pair<u64, u64>> resolve_phi(std::unordered_map<u64, Instr*>& idToPhi);
    void commit_phi(std::unordered_map<u64, Instr*>& idToPhi, bool isIf = false);

    inline u64 get_last_pos() const { return last_instr_pos; }
    inline Instr* get_cmp()
    {
        Instr* cmp = &get_current_block()->instructions[get_current_block()->instructions.size() - 2];
        assert(cmp->type == InstrType::CMP);
        return cmp;
    }

    inline void add_stack(u64 val) { instr_stack.push(val); }
    inline u64 top_stack() const { return instr_stack.top(); }
    inline void pop_stack() { instr_stack.pop(); }
    inline u64 size_stack() const { return instr_stack.size(); }
    inline void clear_stack() { instr_stack = {}; };

    void generate_dot() const;

    std::deque<JoinNodeType> join_stack;

    std::string name = "main";

private:
    SSA(SSA&) = delete;
    u64 last_instr_pos;

    std::stack<u64> instr_stack;

    // InputNum, OutputNum, OutputNewLine
    SymbolTableType symbol_table = { { FUNC_INPUT_NUM, { "read", std::nullopt } }, { FUNC_OUTPUT_NUM, { "write", std::nullopt } }, { FUNC_OUTPUT_NL, { "writeNL", std::nullopt } } };
    const u64 inbuilt_count = symbol_table.size();

    std::unordered_map<Instr, u64> expressions;
    std::unordered_map<u64, u64> constants;

    void add_to_block(Instr instr);
    Instr& add_to_block(Instr&& instr, std::shared_ptr<Block>& b);

    static u64 instruction_num;
    std::shared_ptr<Block> blocks; // head
    std::shared_ptr<Block> current;
    friend std::ostream& operator<<(std::ostream& os, const SSA& ssa);
};
