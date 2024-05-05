#pragma once

#include <stack>

#include "token.h"

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
    u64 id;
    InstrType type;
    u64 x, y;
    friend std::ostream& operator<<(std::ostream& os, const Instr& instr);
};

class Block {
    static u64 __id;
    u64 block_id;

public:
    std::unique_ptr<Block> left, right, parent;

    Block()
        : block_id(__id++)
    {
    }

    inline u64 get_block_id() { return block_id; }

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
    SSA()
        : blocks(std::make_unique<Block>()) {};

    void add_const(u64 val);
    void add_instr(InstrType type);
    Instr& get_curr();

    void add_symbols(const std::vector<u64>& s);
    void set_symbol(Token* t);
    u64 get_symbol(Token* t);

private:
    std::stack<Instr> instructions;

    // "InputNum", "OutputNum", "OutputNewLine" ?
    std::vector<std::optional<u64>> symbol_table = { 0, 0, 0};

    static u64 instruction_num;
    std::unique_ptr<Block> blocks;
    friend std::ostream& operator<<(std::ostream& os, const SSA& ssa);
};
