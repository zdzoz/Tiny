#pragma once

#include <filesystem>

#define __PRTOK(os, tok)                       \
    do {                                       \
        os << token_str[(u64)tok.type()];      \
        if (tok.val())                         \
            os << " " << *tok.val();           \
        if (!tok.id().empty())                 \
            os << " (\"" << tok.id() << "\")"; \
    } while (0)

#define PRTOK(tok) __PRTOK(std::cerr, tok)

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

#define TOKEN_TYPE_COUNT ((u64)TokenType::MAX_TOKEN_TYPE)

enum class TokenType;

class Token {
    TokenType _type;
    std::optional<u64> _val;
    std::string _id;

public:
    Token(TokenType type, std::optional<u64> val);
    Token(TokenType type, std::optional<u64> val, std::string&& id);

    TokenType type() const { return this->_type; }
    inline std::optional<u64> val() const { return this->_val; }
    const std::string& id() const { return this->_id; }

    friend std::ostream& operator<<(std::ostream& os, const Token& tok);
    static bool tokenize(std::filesystem::path file, std::vector<Token>& toks);
};

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

    // OPERATORS
    PLUS,
    MIN,
    DIV,
    MUL,
    LT,
    GT,

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

    // OPERATORS
    "PLUS",
    "MIN",
    "DIV",
    "MUL",
    "LT",
    "GT",

    // MISC
    "ID",
    "NUM",

    "UNKNOWN",
};
