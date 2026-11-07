#pragma once

#include "token.hpp"
#include "errors.hpp"

#include <cstddef>
#include <vector>
#include <cstring>

namespace aet {

// Converts source code into a flat, comma-separated token stream.
class lexer {
private:
    string_view code;
    std::string error;
    std::size_t pos = 0;
    char current_char = '\0';
    char next_char = '\0';
    unsigned current_line = 1;

public:
    explicit lexer(string_view code) : code(code) {
        current_char = code.empty() ? '\0' : code[0];
        next_char = code.size() > 1 ? code[1] : '\0';
    }

    bool at_end() const {
        return pos >= code.size();
    }
    void move_next() {
        if (current_char == '\n') {
            ++current_line;
        }
        if (pos + 1 < code.size()) {
            ++pos;
            current_char = code[pos];
            next_char = (pos + 1 < code.size()) ? code[pos + 1] : '\0';
        } else {
            pos = code.size();
            current_char = '\0';
            next_char = '\0';
        }
    }

    void move_next(short t) {
        if (t > 0) {
            for (short i = 0; i < t; ++i) {
                move_next();
            }
        } else if (t < 0) {
            pos += t;
            if (pos > code.size()) {
                pos = code.size();
            }
            move_next();
        }
    }

    std::string skip_until(const char *expected, const char *escape, const char *unexpected, const bool eof_terminates) {
        std::string value;
        int start_line = current_line;
        auto in = [](const char *set, char c) {
            return c != '\0' && set != nullptr && std::strchr(set, c) != nullptr;
        };
        while (true) {
            if (in(escape, current_char)) {
                value += current_char;
            } else if (in(unexpected, current_char)) {
                error = unexpected_char_error(1, current_char, start_line, current_line).er();
                break;
            } else if (in(expected, current_char)) {
                break;
            } else {
                value += current_char;
            }
            if (next_char == '\0') {
                if (eof_terminates) {
                    move_next();
                } else {
                    error = expected_char_error(1, expected, start_line, current_line).er();
                }
                break;
            }
            move_next();
        }
        return value;
    }

    std::vector<token> make_tokens() {
        std::vector<token> tokens;
        std::vector<char> in_som_list; // stack of open brackets
        while (!at_end() && error.empty()) {
            unsigned start_line = current_line;
            std::string value;
            unsigned short length = 0; // 0 means value already holds the full text
            tokt type = tokt::unknown;

            switch (current_char) {
            case '+': {
                if(next_char == '=') {
                    length = 2;
                    type = tokt::pluseq;
                } else {
                    length = 1;
                    type = tokt::plus;
                }
                break;
            }
            case '-': {
                if(next_char == '=') {
                    length = 2;
                    type = tokt::minuseq;
                } else {
                    length = 1;
                    type = tokt::minus;
                }
                break;
            }
            case '*': {
                if(next_char == '=') {
                    length = 2;
                    type = tokt::muleq;
                } else {
                    length = 1;
                    type = tokt::mul;
                }
                break;
            }
            case '/': {
                if (next_char == '=') {
                    type = tokt::diviseq;
                    length = 2;
                } else if (next_char == '/') {
                        type = tokt::comment;
                        value = skip_until("\n", "\\", nullptr, true) + '\b';
                        move_next();
                } else if (next_char == '*') {
                    type = tokt::multi_line_comment;
                    value = "/*";
                    move_next(2);
                    bool closed = false;
                    while (!at_end() && error.empty()) {
                        if (current_char == '*' && next_char == '/') {
                            value += "*/";
                            move_next();
                            move_next();
                            closed = true;
                            break;
                        }
                        value += current_char;
                        move_next();
                    }
                    if (!closed) {
                        error = expected_char_error(1, "*/", start_line, current_line).er();
                    }
                    move_next(-1);
                } else {
                    length = 1;
                    type = tokt::divis;
                }
                break;
            }
            case '\t':
            case ' ':
            case '\n': {
                move_next();
                continue;
            }
            case '=': {
                if (next_char == '=') {
                    length = 2;
                    type = tokt::iseq;
                } else {
                    type = tokt::eq;
                    length = 1;
                }
                break;
            }
            case '!': {
                if (next_char == '=') {
                    length = 2;
                    type = tokt::isnoteq;
                } else if(next_char == '&') {
                    length = 2;
                    type = tokt::nand;
                } else if (next_char == '|') {
                    length = 2;
                    type = tokt::nor;
                } else {
                    length = 1;
                    type = tokt::nott;
                }
                break;
            }

            case '&': {
                if(next_char == '&') {
                    length = 2;
                    type = tokt::andd;
                } else {
                    length = 1;
                    type = tokt::adr;
                }
                break;
            }

            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9': {
                bool has_dot = false;
                while (!at_end() && std::strchr("0123456789.fd", current_char)) {
                    if (current_char == '.') {
                        if (has_dot) {
                            break;
                        } else {
                            has_dot = true;
                        }
                    } else if (std::strchr("fd", current_char)) {
                        value += current_char;
                        move_next();
                        break;
                    }
                    value += current_char;
                    move_next();
                }
                if (!value.empty() &&
                    (std::strchr("fd", value.back()) || has_dot)) {
                    if (value.back() == 'f') {
                        type = tokt::vt_float;
                    } else {
                        type = tokt::vt_double;
                    }
                } else {
                    type = tokt::vt_int;
                }
                break;
            }
            case '"': {
                value += current_char;
                move_next();
                value += skip_until("\"", "\\", "\n", true);
                value += current_char;
                move_next();
                type = tokt::vt_string;
                break;
            }
            case '\'': {
                type = tokt::vt_char;
                const unsigned short body = next_char == '\\' ? 2 : 1;
                if (next_char == '\0' || next_char == '\'') {
                    length = next_char == '\'' ? 2 : 1;
                    error = expected_char_error(1, '\'', start_line, current_line).er();
                } else if (pos + body + 1 < code.size() &&
                        code[pos + body + 1] == '\'') {
                    length = body + 2;
                    value = std::string(code.substr(pos, length));
                } else {
                    length = 1 + body;
                    value = std::string(code.substr(pos, length));
                    error = expected_char_error(1, '\'', start_line, current_line).er();
                }
                break;
            }
            case ',': {
                length = 1;
                type = tokt::comma;
                break;
            }
            case '{': {
                length = 1;
                type = tokt::Lbrace;
                in_som_list.push_back(current_char);
                break;
            }
            case '}': {
                length = 1;
                type = tokt::Rbrace; // Добавлена ;
                if (!in_som_list.empty() && in_som_list.back() == '{') {
                    in_som_list.pop_back();
                } else {
                    error = expected_char_error(1, '[', start_line, current_line).er(); 
                }
                break;
            }
            case '[': {
                length = 1;
                type = tokt::Lbracket;
                in_som_list.push_back(current_char);
                break;
            }
            case ']': {
                length = 1;
                type = tokt::Rbracket;
                if (!in_som_list.empty() && in_som_list.back() == '[') {
                    in_som_list.pop_back();
                } else {
                    error = expected_char_error(1, '[', start_line, current_line).er(); 
                }
                break;
            }
            case '(': {
                length = 1;
                type = tokt::Lparen;
                in_som_list.push_back(current_char);
                break;
            }
            case ')': {
                length = 1;
                type = tokt::Rparen;
                if (!in_som_list.empty() && in_som_list.back() == '(') {
                    in_som_list.pop_back();
                } else {
                    error = expected_char_error(1, '(', start_line, current_line).er(); 
                }
                break;
            }
                case ';': {
                    length = 1;
                    type = tokt::semicolon;
                    break;
                }
                case '#': {
                    length = 1;
                    type = tokt::hash;
                    break;
                }
            default: {
                type = tokt::command;
                const std::size_t command_start = pos;
                value = skip_until(" ,\n(:){;}[]", nullptr, nullptr, true);
                if (pos == command_start) {
                    value = std::string(1, current_char);
                    length = 1;
                }
                if(value == "true" || value == "false") {type = tokt::vt_bool;}
                break;
            }
            }

            if (!error.empty()) {
                break;
            }

            if (value.empty()) {
                if (length == 1) {
                    value = current_char;
                } else if (length == 2) {
                    value = std::string(1, current_char) + next_char;
                }
            }
            tokens.push_back(token(type, value));
            move_next(length);
        }
        if (!error.empty()) {
            tokens.push_back(token(tokt::error, error));
        }
        return tokens;
    }
};
std::string run_tokens_as_string(std::string_view Code) {
    std::vector<token> tok_tokens = lexer(Code).make_tokens();

    std::vector<std::string> tokens;
    tokens.reserve(tok_tokens.size());

    for (std::size_t i = 0; i < tok_tokens.size(); ++i) {
        tokens.push_back(tok_tokens[i].token_as_string());
    }

    return join(tokens);
}

std::vector<token> run_tokens(std::string_view Code) {
    return aet::lexer(Code).make_tokens();
}
}