#pragma once

#include <filesystem>

#define __PRTOK(os, tok)                         \
    do {                                         \
        os << token_str[(u64)(tok).type()];      \
        if ((tok).val())                         \
            os << " " << *(tok).val();           \
        if (!(tok).id().empty())                 \
            os << " (\"" << (tok).id() << "\")"; \
    } while (0)

#define PRTOK(tok) __PRTOK(std::cerr, (tok))
#define PRTOKLN(tok)     \
    do {                 \
        PRTOK((tok));    \
        LOG_ERROR("\n"); \
    } while (0)

#define PRINT_TOKENS(os, toks)                     \
    do {                                           \
        for (size_t i = 0; i < toks.size(); i++) { \
            const auto& e = toks[i];               \
            os << e;                               \
            if (i != toks.size() - 1)              \
                os << ", ";                        \
        }                                          \
        os << "\n";                                \
    } while (0)

enum class TokenType {
    // KEYWORDS
    MAIN,
    CALL,
    RET,
    LET,
    VAR,
    IF,
    THEN,
    ELSE,
    FI,
    WHILE,
    DO,
    OD,

    // DELIMITERS
    PERIOD,
    COMMA,
    SEMI,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    ASSIGN,

    // RELATION - == | != | < | <= | > | >=
    EQ,
    NEQ,
    LT,
    LTEQ,
    GT,
    GTEQ,

    // OPERATORS
    PLUS,
    MIN,
    DIV,
    MUL,

    // MISC
    ID,
    NUM,

    UNKNOWN,
    MAX_TOKEN_TYPE,
};

const std::vector<std::string> token_str = {
    "MAIN",
    "CALL",
    "RETURN",
    "LET",
    "VAR",
    "IF",
    "THEN",
    "ELSE",
    "FI",
    "WHILE",
    "DO",
    "OD",

    // DELIMITERS
    "PERIOD",
    "COMMA",
    "SEMI",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "ASSIGN",

    // RELATION
    "EQ",
    "NEQ",
    "LT",
    "LTEQ",
    "GT",
    "GTEQ",

    // OPERATORS
    "PLUS",
    "MIN",
    "DIV",
    "MUL",

    // MISC
    "ID",
    "NUM",

    "UNKNOWN",
};

#define TOKEN_TYPE_COUNT ((u64)TokenType::MAX_TOKEN_TYPE)

enum class TokenType;

class Token {
    TokenType _type;
    std::optional<u64> _val;
    std::string _id;

public:
    Token();
    Token(TokenType type, std::optional<u64> val);
    Token(TokenType type, std::optional<u64> val, std::string&& id);

    TokenType type() const { return this->_type; }
    inline std::optional<u64> val() const { return this->_val; }
    const std::string& id() const { return this->_id; }

    friend std::ostream& operator<<(std::ostream& os, const Token& tok);
};

class TokenList {
    std::vector<Token> toks;
    uint64_t index;

public:
    bool tokenize(std::filesystem::path file);
    void show();
    inline size_t size() const { return toks.size(); }
    inline size_t remaining() const { return toks.size() - index; }

    inline TokenType get_type() const
    {
        if (index < toks.size())
            return toks[index].type();
        return TokenType::UNKNOWN;
    }

    inline const Token* get() const
    {
        if (index < toks.size())
            return &toks[index];
        return nullptr;
    }

    void eat() { index++; }

    // std::optional<const Token*> get() const {
    //     if (index >= toks.size()) return {};
    //     return &toks[index];
    // }
};
