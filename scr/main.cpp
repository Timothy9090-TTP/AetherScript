#include "lexer.hpp"
#include "parser.hpp"
#include "translator.hpp"

#include <iostream>

int main() {
    std::string code;
    std::cout << "> ";
    std::string mode = "run";
    while (std::getline(std::cin, code)) {
        if (code == "exit") break;
        if(code == "tok" || code == "tre" || code == "run") {
            mode = code;
            if(mode == "run") std::cout << "\n> ";
            else if(mode == "tok") std::cout << "\n➜ ";
            else if(mode == "tre") std::cout << "\n/ ";
            continue;
        }
        if(mode == "run") std::cout << "< " << aet::run_translator(aet::run_tree(aet::run_tokens(code))) << " \n> ";
        else if(mode == "tok") std::cout << "= " << aet::run_tokens_as_string(code) << "\n➜ ";
        else if(mode == "tre") std::cout << "\\ " << aet::run_tree_as_string(aet::run_tokens(code)) << "\n/ ";
    }
    return 0;
}