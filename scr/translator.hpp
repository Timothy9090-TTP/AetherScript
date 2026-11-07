#pragma once

#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace aet {

class translation_error : public std::runtime_error {
public:
    explicit translation_error(std::string message)
        : std::runtime_error(std::move(message)) {}
};

class translator {
public:
    explicit translator(nlohmann::json tree) : tree_(std::move(tree)) {}
    [[nodiscard]] std::string translate() const;

    [[nodiscard]] std::string translate_with_headers() const;

private:
    nlohmann::json tree_;

    // ------------------------------------------------------------------
    // Expression generators
    // ------------------------------------------------------------------

    [[nodiscard]] std::string generate_expression(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_literal(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_identifier(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_call(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_binop(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_unary(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_array(
        const nlohmann::json& node) const;

    // ------------------------------------------------------------------
    // Statement generators
    // ------------------------------------------------------------------

    [[nodiscard]] std::string generate_statement(
        const nlohmann::json& stmt) const;

    [[nodiscard]] std::string generate_block(
        const nlohmann::json& node) const;

    [[nodiscard]] std::string generate_directive(
        const nlohmann::json& node) const;

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    [[nodiscard]] static std::string cpp_operator(std::string_view op)
        noexcept;

    [[nodiscard]] static std::string escape_cpp_string(
        std::string_view s) noexcept;

    [[nodiscard]] static std::string indent_string(int level) noexcept;
};


inline std::string translator::cpp_operator(std::string_view op) noexcept {
    static const std::unordered_map<std::string_view, std::string_view> kMap{
        {"eq", "="},          {"iseq", "=="},      {"isnoteq", "!="},
        {"pluseq", "+="},     {"minuseq", "-="},   {"muleq", "*="},
        {"diviseq", "/="},    {"powareq", "^="},   {"plus", "+"},
        {"minus", "-"},       {"mul", "*"},        {"divis", "/"},
        {"powar", "^"},       {"andd", "&&"},      {"orr", "||"},
        {"nott", "!"},      {"not", "!"},       {"norr", "!"},
    };
    auto it = kMap.find(op);
    return (it != kMap.end()) ? std::string(it->second) : std::string(op);
}

inline std::string translator::escape_cpp_string(std::string_view s) noexcept {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

inline std::string translator::indent_string(int level) noexcept {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

inline std::string translator::generate_literal(
    const nlohmann::json& node) const {
    std::string subtype = node.value("subtype", std::string{});
    std::string value   = node.value("value",   std::string{});

    if (subtype == "vt_string") {
        return "\"" + escape_cpp_string(value) + "\"";
    }
    if (subtype == "vt_bool") {
        return (value == "true")  ? "true"
             : (value == "false") ? "false"
                                  : value;
    }
    return value;
}

inline std::string translator::generate_identifier(
    const nlohmann::json& node) const {
    return node.value("name", std::string{});
}

inline std::string translator::generate_call(
    const nlohmann::json& node) const {
    std::string callee = node.value("callee", std::string{});
    std::string out    = callee + "(";

    const auto& args = node.value("args", nlohmann::json::array());
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) out += ", ";
        out += generate_expression(args[i]);
    }
    out += ")";
    return out;
}

inline std::string translator::generate_binop(
    const nlohmann::json& node) const {
    std::string op   = cpp_operator(node.value("op", std::string{}));
    std::string left = generate_expression(node.value("left", nlohmann::json{}));
    std::string right =
        generate_expression(node.value("right", nlohmann::json{}));
    return "(" + left + " " + op + " " + right + ")";
}

inline std::string translator::generate_unary(
    const nlohmann::json& node) const {
    std::string op      = cpp_operator(node.value("op", std::string{}));
    std::string operand =
        generate_expression(node.value("operand", nlohmann::json{}));
    return "(" + op + operand + ")";
}

inline std::string translator::generate_array(
    const nlohmann::json& node) const {
    std::string out = "{";
    const auto& elems = node.value("elements", nlohmann::json::array());
    for (size_t i = 0; i < elems.size(); ++i) {
        if (i != 0) out += ", ";
        out += generate_expression(elems[i]);
    }
    out += "}";
    return out;
}

inline std::string translator::generate_expression(
    const nlohmann::json& node) const {
    if (!node.is_object()) {
        return "";
    }

    std::string type = node.value("type", std::string{});

    if (type == "literal")     return generate_literal(node);
    if (type == "identifier")  return generate_identifier(node);
    if (type == "call")        return generate_call(node);
    if (type == "binop")       return generate_binop(node);
    if (type == "unary")       return generate_unary(node);
    if (type == "array")       return generate_array(node);
    if (type == "error") {
        return "// error: " + node.value("message", std::string{"unknown"});
    }

    throw translation_error("unknown expression node type: '" + type + "'");
}

inline std::string translator::generate_statement(
    const nlohmann::json& stmt) const {
    if (!stmt.is_object()) {
        return "";
    }

    std::string type = stmt.value("type", std::string{});

    if (type == "empty_stmt") {
        return "";
    }

    if (type == "expr_stmt") {
        return generate_expression(stmt.value("expression", nlohmann::json{}))
             + ";\n";
    }

    if (type == "call") {
        return generate_call(stmt) + ";\n";
    }

    if (type == "block") {
        return generate_block(stmt);
    }

    if (type == "directive") {
        return generate_directive(stmt);
    }

    if (type == "error") {
        return "// error: " + stmt.value("message", std::string{"unknown"})
             + "\n";
    }

    throw translation_error("unknown statement node type: '" + type + "'");
}

inline std::string translator::generate_block(
    const nlohmann::json& node) const {
    std::string out = "{\n";
    const auto& body = node.value("body", nlohmann::json::array());
    for (const auto& stmt : body) {
        out += "  " + generate_statement(stmt);
    }
    out += "}\n";
    return out;
}

inline std::string translator::generate_directive(
    const nlohmann::json& node) const {
    std::string name = node.value("name", std::string{});
    std::string out  = "#" + name + " {\n";
    const auto& body = node.value("body", nlohmann::json::array());
    for (const auto& stmt : body) {
        out += "  " + generate_statement(stmt);
    }
    out += "}\n";
    return out;
}

inline std::string translator::translate() const {
    std::ostringstream out;

    std::string type = tree_.value("type", std::string{});

    if (type == "prg") {
        const auto& body = tree_.value("body", nlohmann::json::array());
        for (const auto& stmt : body) {
            out << generate_statement(stmt);
        }
    } else {
        out << generate_statement(tree_);
    }

    return out.str();
}

inline std::string translator::translate_with_headers() const {
    std::ostringstream out;
    out << "#include <iostream>\n"
           "#include <string>\n"
           "#include <vector>\n"
           "using namespace std;\n\n"
           "int main() {\n";
    out << translate();
    out << "  return 0;\n}\n";
    return out.str();
}

inline std::string run_translator(const nlohmann::json& tree) {
    try {
        translator tr(tree);
        return tr.translate();
    } catch (const std::exception& e) {
        std::cerr << "translation error: " << e.what() << std::endl;
        return "";
    }
}

}  // namespace aet
