#pragma once

#include "token.hpp"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

namespace aet {

class parser {
    private:
        std::vector<token> tokens;
        size_t pos = 0;

    public:
        parser(std::vector<token> Tokens)
            : tokens(std::move(Tokens)), pos(0) {}

        bool at_end() const { return pos >= tokens.size(); }

        const token& peek(size_t off = 0) const { return tokens[pos + off]; }
        
        void advance() { if (!at_end()) ++pos; }

        bool match(tokt t) {
            if (at_end()) return false;
            if (peek().get_type() == t) { advance(); return true; }
            return false;
        }

    private:
        static nlohmann::json node(std::string type) {
            nlohmann::json n;
            n["type"] = std::move(type);
            return n;
        }

        static int bin_prec(tokt t) {
            switch (t) {
                case tokt::eq: case tokt::pluseq: case tokt::minuseq:
                case tokt::muleq: case tokt::diviseq: case tokt::powareq:
                    return 1;
                case tokt::orr: case tokt::nor: case tokt::nand:
                    return 2;
                case tokt::andd:
                    return 3;
                case tokt::iseq: case tokt::isnoteq:
                    return 4;
                case tokt::plus: case tokt::minus:
                    return 5;
                case tokt::mul: case tokt::divis:
                    return 6;
                case tokt::powar:
                    return 8;
                default:
                    return 0;
            }
        }

        static bool right_assoc(tokt t) {
            return t == tokt::eq || t == tokt::pluseq || t == tokt::minuseq ||
                   t == tokt::muleq || t == tokt::diviseq || t == tokt::powareq ||
                   t == tokt::powar;
        }

        nlohmann::json parse_expression(int min_prec = 0) {
            if (!at_end()) {
                tokt t = peek().get_type();
                if (t == tokt::nott || t == tokt::minus) {
                    advance();
                    nlohmann::json n = node("unary");
                    n["op"] = tokt_name(t);
                    n["operand"] = parse_expression(0);
                    return n;
                }
            }

            nlohmann::json left = parse_primary();
            while (!at_end()) {
                tokt t = peek().get_type();
                int prec = bin_prec(t);
                if (prec < min_prec || prec == 0) break;
                advance();
                nlohmann::json n = node("binop");
                n["op"] = tokt_name(t);
                n["left"] = left;
                n["right"] = parse_expression(right_assoc(t) ? prec : prec + 1);
                left = n;
            }
            return left;
        }

        nlohmann::json parse_primary() {
            if (at_end()) {
                nlohmann::json n = node("error");
                n["message"] = "unexpected end of input";
                return n;
            }

            tokt t = peek().get_type();

            switch (t) {
                case tokt::vt_int:
                case tokt::vt_float:
                case tokt::vt_double:
                case tokt::vt_bool:
                case tokt::vt_string:
                case tokt::vt_char: {
                    nlohmann::json n = node("literal");
                    n["value"] = peek().get_value();
                    n["subtype"] = tokt_name(t);
                    advance();
                    return n;
                }
                case tokt::command: {
                    std::string name = peek().get_value();
                    advance();
                    if (match(tokt::Lparen)) {
                        nlohmann::json n = node("call");
                        n["callee"] = name;
                        n["args"] = nlohmann::json::array();
                        if (!match(tokt::Rparen)) {
                            while (!at_end()) {
                                n["args"].push_back(parse_expression(0));
                                if (match(tokt::comma)) continue;
                                if (match(tokt::Rparen)) break;
                                break;
                            }
                        }
                        return n;
                    }
                    nlohmann::json n = node("identifier");
                    n["name"] = name;
                    return n;
                }
                case tokt::Lparen: {
                    advance();
                    nlohmann::json n = parse_expression(0);
                    match(tokt::Rparen);
                    return n;
                }
                case tokt::Lbracket: {
                    advance();
                    nlohmann::json n = node("array");
                    n["elements"] = nlohmann::json::array();
                    if (!match(tokt::Rbracket)) {
                        while (!at_end()) {
                            n["elements"].push_back(parse_expression(0));
                            if (match(tokt::comma)) continue;
                            match(tokt::Rbracket);
                            break;
                        }
                    }
                    return n;
                }
                default: {
                    nlohmann::json n = node("error");
                    n["message"] = "unexpected token: " +
                        std::string(tokt_name(t)) + ":" + peek().get_value();
                    advance();
                    return n;
                }
            }
        }

        nlohmann::json parse_directive() {
            match(tokt::hash);
            nlohmann::json n = node("directive");
            if (!at_end() && peek().get_type() == tokt::command) {
                n["name"] = peek().get_value();
                advance();
            } else {
                n["name"] = "";
            }
            if (match(tokt::Lbrace)) {
                n["body"] = nlohmann::json::array();
                while (!at_end() && peek().get_type() != tokt::Rbrace) {
                    n["body"].push_back(parse_statement());
                }
                match(tokt::Rbrace);
            }
            return n;
        }

        nlohmann::json parse_statement() {
            if (match(tokt::semicolon)) {
                return node("empty_stmt");
            }

            if (match(tokt::Lbrace)) {
                nlohmann::json n = node("block");
                n["body"] = nlohmann::json::array();
                while (!at_end() && peek().get_type() != tokt::Rbrace) {
                    n["body"].push_back(parse_statement());
                }
                match(tokt::Rbrace);
                return n;
            }

            nlohmann::json n = node("expr_stmt");
            n["expression"] = parse_expression(0);
            match(tokt::semicolon);
            return n;
        }

    public:
        nlohmann::json make_tree() {
            nlohmann::json tree;
            tree["type"] = "prg";
            tree["body"] = nlohmann::json::array();
            while (!at_end()) {
                tokt t = peek().get_type();
                if (t == tokt::semicolon || t == tokt::command ||
                    t == tokt::vt_int || t == tokt::vt_float ||
                    t == tokt::vt_double || t == tokt::vt_bool ||
                    t == tokt::vt_string || t == tokt::vt_char ||
                    t == tokt::hash || t == tokt::nott || t == tokt::minus ||
                    t == tokt::Lparen || t == tokt::Lbracket ||
                    t == tokt::Lbrace) {
                    if (t == tokt::hash) {
                        tree["body"].push_back(parse_directive());
                    } else {
                        tree["body"].push_back(parse_statement());
                    }
                } else if (t == tokt::Rbrace || t == tokt::Rparen ||
                           t == tokt::Rbracket) {
                    break;
                } else {
                    advance();
                }
            }
            return tree;
        }
};

std::string run_tree_as_string(std::vector<token> code) {
    parser p(code);
    return p.make_tree().dump(2);
}

nlohmann::json run_tree(std::vector<token> code) {
    return parser(code).make_tree();
}

}
