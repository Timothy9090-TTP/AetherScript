#pragma once

#include "token.hpp"
#include <utility>

namespace aet {
    class error {
        protected:
            std::string error_code;
            std::string error_name;
            std::string error_details;

        public:
            error(std::string code, std::string name, std::string details)
                : error_code(std::move(code)), error_name(std::move(name)),
            error_details(std::move(details)) {}
            ~error() = default;

            std::string er() const {
                if (error_details.empty()) {
                    return "ER" + error_code + ", " + error_name;
                } else {
                    return "ER" + error_code + ", " + error_name + ": " + error_details;
                }
            }
    };

    class illegal_char_error : public error {
        public:
            explicit illegal_char_error(const std::string &details)
                : error("1", "illegal character", details) {}

            template <typename T>
            illegal_char_error(unsigned short templata, T &&cchar,
                            unsigned start_line = 0, unsigned end_line = 0)
                : error("1", "illegal character(s)",
                        error_template(templata, cchar, end_line, start_line)) {}
    };

    class unexpected_char_error : public error {
        public:
            explicit unexpected_char_error(const std::string &details)
                : error("2", "unexpected character(s)", details) {}

            template <typename T>
            unexpected_char_error(unsigned short templata, T &&cchar,
                                unsigned start_line = 0, unsigned end_line = 0)
                : error("2", "unexpected character(s)",
                        error_template(templata, cchar, end_line, start_line)) {}
    };

    class expected_char_error : public error {
        public:
            explicit expected_char_error(const std::string &details)
                : error("3", "expected character(s)", details) {}

            template <typename T>
            expected_char_error(unsigned short templata, T &&cchar,
                                unsigned start_line = 0, unsigned end_line = 0)
                : error("3", "expected character(s)",
                        error_template(templata, cchar, end_line, start_line)) {}
    };

}