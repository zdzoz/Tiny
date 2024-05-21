#pragma once

#include <stack>
#ifndef NDEBUG
#include <iostream>
#endif

#include "token.h"

// InputNum, OutputNum, OutputNewLine
#define FUNC_INPUT_NUM 0
#define FUNC_OUTPUT_NUM 1
#define FUNC_OUTPUT_NL 2

typedef std::pair<std::string, std::optional<u64>> SymbolType;
typedef std::vector<SymbolType> SymbolTableType;

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

    END,
    NONE,
    UNKNOWN,
};

struct Instr {
    u64 pos;
    InstrType type;
    std::optional<u64> x, y;
    friend std::ostream& operator<<(std::ostream& os, const Instr& instr);
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
    inline Instr& front() { return instructions.front(); }
    inline Instr& back() { return instructions.back(); }
    inline void pop_back() { instructions.pop_back(); }

private:
    // NOTE: maybe change to std::deque instead of vector (allows references to items)
    std::deque<Instr> instructions;
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

    void add_const(u64 val);
    void add_instr(InstrType type);

    // void add_symbols(u64 count);
    void add_symbols(SymbolTableType&& v);
    void set_symbol(const Token* t);
    void add_symbols_to_block(JoinNodeType& join_node);

    bool resolve_symbol(const Token* t);
    void print_symbol_table();

    inline void add_stack(u64 val) { instr_stack.push(val); }
    void resolve_branch(std::shared_ptr<Block>& from, std::shared_ptr<Block>& to);

    void resolve_phi(std::unordered_map<u64, Instr*>& idToPhi);
    void commit_phi(std::unordered_map<u64, Instr*>& idToPhi);

    inline u64 get_last_pos() const { return last_instr_pos; }
    inline Instr* get_cmp() {
        Instr* cmp = &get_current_block()->instructions[get_current_block()->instructions.size() - 2];
        assert(cmp->type == InstrType::CMP);
        return cmp;
    }

    void clear_stack() { instr_stack = {}; };

    void generate_dot() const;

    std::deque<JoinNodeType> join_stack;
private:
    SSA(SSA&) = delete;
    u64 last_instr_pos;

    std::stack<u64> instr_stack;

    // InputNum, OutputNum, OutputNewLine
    SymbolTableType symbol_table = { { "read", std::nullopt }, { "write", std::nullopt }, { "writeNL", std::nullopt } };
    const u64 inbuilt_count = symbol_table.size();

    void add_to_block(Instr&& instr);
    Instr& add_to_block(Instr&& instr, std::shared_ptr<Block>& b);

    static u64 instruction_num;
    std::shared_ptr<Block> blocks; // head
    std::shared_ptr<Block> current;
    friend std::ostream& operator<<(std::ostream& os, const SSA& ssa);
};
