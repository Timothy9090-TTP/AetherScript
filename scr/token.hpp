#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <ranges>
#include <type_traits>

namespace aet {

    using string_view = std::string_view;

    enum class tokt {
        error,
        command,
        comma,
        comment,
        multi_line_comment,
        hash,
        vt_string,
        vt_char,
        vt_bool,
        vt_double,
        vt_float,
        vt_int,
        Lparen,
        Rparen,
        Lbrace,
        Rbrace,
        Lbracket,
        Rbracket,
        eq,
        iseq,
        isnoteq,
        pluseq,
        minuseq,
        muleq,
        diviseq,
        powareq,
        semicolon,
        plus,
        minus,
        mul,
        divis,
        powar,
        nott,
        andd,
        orr,
        nor,
        nand,
        adr,
        unknown
    };

        std::string_view tokt_name(tokt type) {
        switch (type) {
            case tokt::multi_line_comment: return "multi_line_comment";
            case tokt::comment:            return "comment";
            case tokt::hash:               return "hash";
            case tokt::error:              return "error";
            case tokt::command:            return "command";
            case tokt::vt_string:          return "vt_string";
            case tokt::vt_char:            return "vt_char";
            case tokt::vt_bool:            return "vt_bool";
            case tokt::vt_double:          return "vt_double";
            case tokt::vt_float:           return "vt_float";
            case tokt::vt_int:             return "vt_int";
            case tokt::plus:               return "plus";
            case tokt::minus:              return "minus";
            case tokt::mul:                return "mul";
            case tokt::divis:              return "divis";
            case tokt::powar:              return "powar";                
            case tokt::Lparen:             return "Lparen";
            case tokt::Rparen:             return "Rparen";
            case tokt::Lbrace:             return "Lbrace";
            case tokt::Rbrace:             return "Rbrace";
            case tokt::Lbracket:           return "Lbracket";
            case tokt::Rbracket:           return "Rbracket";
            case tokt::eq:                 return "eq";
            case tokt::iseq:               return "iseq";
            case tokt::isnoteq:            return "isnoteq";
            case tokt::pluseq:             return "pluseq";
            case tokt::minuseq:            return "minuseq";
            case tokt::muleq:              return "muleq";
            case tokt::diviseq:            return "diviseq";
            case tokt::powareq:            return "powareq";
            case tokt::unknown:            return "unknown";
            case tokt::comma:              return "comma";
            case tokt::semicolon:          return "semicolon";
            case tokt::nott:               return "not";
            case tokt::andd:               return "and";
            case tokt::orr:                return "or";
            case tokt::nor:                return "nor";
            case tokt::nand:               return "nand";
            case tokt::adr:                return "adr";
        }
        return "";
    }

    template <typename T>
    std::string to_str(const T &value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }

    template <typename Container>
    std::string join(const Container &c, std::string sep = ", ") {
        std::string result;
        result.reserve(64);
        bool first = true;
        for (const auto &x : c) {
            if (!first)
                result += sep;
            first = false;
            if constexpr (std::ranges::range<std::remove_cvref_t<decltype(x)>>) {
                std::string s = x;
                for (std::size_t p = s.find_first_of("\n\t"); p != std::string::npos;
                    p = s.find_first_of("\n\t", p + 2)) {
                    s.replace(p, 1, s[p] == '\n' ? "\\n" : "\\t");
                }
                result += s;
            } else {
                if (x == '\n')
                    result += "\\n";
                else if (x == '\t')
                    result += "\\t";
                else
                    result += x;
            }
        }
        return result;
    }

    template <typename T>
    std::string error_template(const unsigned short layout, const T &cchar,
                            const int line_end = 0, const int line_start = 0) {
        std::string loc, err, cchars;
        if constexpr (std::is_same_v<T, std::string>) {
            cchars = cchar;
        } else if constexpr (std::is_same_v<std::decay_t<T>, const char *> ||
                            std::is_same_v<std::decay_t<T>, char *>) {
            cchars = join(std::string_view(cchar), "");
        } else if constexpr (std::is_same_v<std::decay_t<T>, char>) {
            cchars = join(std::string_view(&cchar, 1), "");
        } else if constexpr (std::ranges::range<std::remove_cvref_t<T>>) {
            cchars = join(cchar, " or ");
        } else {
            cchars = to_str(cchar);
        }

        if (line_start == 0 && line_end == 0) {
            loc = "";
        } else if (line_end == 0) {
            loc = "from line: " + std::to_string(line_start);
        } else if (line_start == 0) {
            loc = "in line: " + std::to_string(line_end);
        } else {
            loc = "in lines: " + std::to_string(line_start) + "-" + std::to_string(line_end);
        }

        switch (layout) {
            case 1:
                err = "'" + cchars + "' " + loc;
                break;
            default:
                err = std::string("(unknown error) ") + '\'' + cchars + "' " + loc;
                break;
        }
        return err;
    }

    class token {
    private:
        std::string value;
        tokt type;
    public:
        token(tokt type, std::string value)
            : value(std::move(value)), type(type) {}

        const std::string& get_value() const { return value;}
        const tokt& get_type()  const { return type;}
        std::string token_as_string() const {
            return std::string(tokt_name(type)) + ":" + value;
    }
    };
}