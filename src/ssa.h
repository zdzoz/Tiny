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
};

struct Instr {
    u64 pos;
    InstrType type;
    u64 x, y;
    friend std::ostream& operator<<(std::ostream& os, const Instr& instr);
};

class SSA;
class Block {
    static u64 __id;
    u64 block_id;

    friend SSA;

public:
    // NOTE: maybe add sibling property?
    std::shared_ptr<Block> left, right, parent;

    Block()
        : block_id(__id++)
    {
    }

    inline u64 get_block_id() const { return block_id; }

    inline void add(Instr&& instr)
    {
        instructions.emplace_back(instr);
    }

    void print_with_indent(std::ostream& os, u64 indent) const;

private:
    std::vector<Instr> instructions;
    friend std::ostream& operator<<(std::ostream& os, const Block& b);
};

class SSA {
public:
    SSA();

#ifndef NDEBUG
    ~SSA()
    {
        print_symbol_table();
    }
#endif

    void add_block(bool isLeft);

    void add_const(u64 val);
    void add_instr(InstrType type);

    void add_symbols(u64 count);
    void set_symbol(const Token* t, u64 pos);

    bool resolve_symbol(const Token* t);
    void print_symbol_table();

    inline u64 get_last_pos() const { return last_instr_pos; }

private:
    std::stack<u64> options;
    u64 last_instr_pos;

    // InputNum, OutputNum, OutputNewLine
    std::vector<std::optional<u64>> symbol_table = { 0, 0, 0 };

    void add_to_block(Instr&& instr);

    static u64 instruction_num;
    std::shared_ptr<Block> blocks; // head
    std::shared_ptr<Block> current;
    friend std::ostream& operator<<(std::ostream& os, const SSA& ssa);
};
