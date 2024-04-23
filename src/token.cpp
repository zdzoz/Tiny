#include "token.h"

#include <cassert>
#include <fstream>
#include <iostream>

Token::Token(TokenType type, std::optional<u64> val)
    : _type(type)
    , _val(val)
{
}

Token::Token(TokenType type, std::optional<u64> val, std::string&& id)
    : _type(type)
    , _val(val)
    , _id(std::move(id))
{
}

static u64 get_id(const std::string& id);
bool Token::tokenize(std::filesystem::path file, std::vector<Token>& toks)
{
    assert(TOKEN_TYPE_COUNT == token_str.size());
    std::ifstream f;

    f.open(file.c_str(), std::ifstream::in);

    if (f.fail()) {
        f.close();
        return false;
    }

    char c;
    while (f.get(c)) {
        if (isspace(c))
            continue;

        TokenType type = TokenType::UNKNOWN;
        std::optional<u64> val;
        std::string id;
        bool save_id = false;
        switch (c) {
        case '+':
            type = TokenType::PLUS;
            break;
        case '-':
            type = TokenType::MIN;
            break;
        case '*':
            type = TokenType::MUL;
            break;
        case '/':
            type = TokenType::DIV;
            break;
        case '.':
            type = TokenType::PERIOD;
            break;
        case ',':
            type = TokenType::COMMA;
            break;
        case ';':
            type = TokenType::SEMI;
            break;
        case '(':
            type = TokenType::LPAREN;
            break;
        case ')':
            type = TokenType::RPAREN;
            break;
        case '{':
            type = TokenType::LBRACE;
            break;
        case '}':
            type = TokenType::RBRACE;
            break;
        case '>':
            type = TokenType::GT;
            break;
        case '<': {
            char next = f.peek();
            if (next == '-') {
                type = TokenType::ASSIGN;
                f.get();
                break;
            }
            type = TokenType::LT;
            break;
        }
        default:
            if (isdigit(c)) {
                u64 num = c - '0';
                while (isdigit(f.peek())) {
                    c = f.get();
                    num = num * 10 + (c - '0');
                }
                type = TokenType::NUM;
                val = num;
                break;
            } else if (isalpha(c)) {
                id += c;
                while (isalnum(f.peek())) {
                    c = f.get();
                    id += c;
                }

                if (id == "main") {
                    type = TokenType::MAIN;
                } else if (id == "call") {
                    type = TokenType::CALL;
                } else if (id == "return") {
                    type = TokenType::RET;
                } else if (id == "let") {
                    type = TokenType::LET;
                } else if (id == "var") {
                    type = TokenType::VAR;
                } else if (id == "if") {
                    type = TokenType::IF;
                } else if (id == "then") {
                    type = TokenType::THEN;
                } else if (id == "else") {
                    type = TokenType::ELSE;
                } else if (id == "fi") {
                    type = TokenType::FI;
                } else if (id == "while") {
                    type = TokenType::WHILE;
                } else if (id == "do") {
                    type = TokenType::DO;
                } else if (id == "od") {
                    type = TokenType::OD;
                } else { // NOTE: is an identifier
                    type = TokenType::ID;
                    val = get_id(id);
                    save_id = true;
                }
            } else {
                TODO("UNKNOWN");
                save_id = true;
                // const char* temp = str;
                // int count = 0;
                // while (!isspace(*temp) && *temp != 0) {
                //     count++;
                //     temp++;
                // }
                // tok->str = (char*)malloc(sizeof(char) * count + 1);
                // strncpy(tok->str, str, count + 1);
                // tok->str[count] = '\0';
                // str += count - 1;
            }
        }

        if (save_id)
            toks.emplace_back(type, val, std::move(id));
        else
            toks.emplace_back(type, val);
    }

    f.close();
    return true;
}

static u64 get_id(const std::string& id)
{
    static u64 __func_id = 0;
    static std::unordered_map<std::string, u64> __func_map = {{"InputNum", __func_id++}, {"OutputNum", __func_id++}, {"OutputNewLine", __func_id++}};

    if (__func_map.find(id) != __func_map.end()) {
        return __func_map.at(id);
    }

    __func_map[id] = __func_id;
    return __func_id++;
}

std::ostream& operator<<(std::ostream& os, const Token& tok)
{
    __PRTOK(os, tok);

    return os;
}
