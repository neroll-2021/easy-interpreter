#ifndef NEROLL_SCRIPT_DETAIL_LEXER_H
#define NEROLL_SCRIPT_DETAIL_LEXER_H

#include <string_view>      // std::string_view
#include <string>           // std::string
#include <cassert>          // assert
#include <format>           // std::formatter
#include <unordered_map>    // std::unordered_map

#include "script/detail/position_t.h"   // position_t

namespace neroll {

namespace script {

namespace detail {

enum class token_type {
    keyword_int,
    keyword_float,
    keyword_boolean,
    keyword_function,
    keyword_for,
    keyword_while,
    keyword_if,
    keyword_else,
    keyword_return,
    keyword_break,
    keyword_continue,
    literal_true,
    literal_false,
    literal_int,            // 123
    literal_float,          // 1.1, 0.05, 1e-2
    plus,                   // +
    minus,                  // -
    asterisk,               // *
    slash,                  // /
    mod,                    // %
    amp,                    // &
    logical_and,            // &&
    vertical_bar,           // |
    colon,                  // :
    comma,                  // ,
    exclamation,            // !
    logical_or,             // ||
                            // ~
    single_quote,           // '
    double_quote,           // "
    backslash,              // \ (backslash)
    semicolon,              // ;
    assign,                 // =
    equal,                  // ==
    not_equal,              // !=
    less,                   // <
    greater,                // >
    left_parenthese,        // (
    right_parenthese,       // )
    left_bracket,           // [
    right_bracket,          // ]
    left_brace,             // {
    right_brace,            // }
    identifier,
    end_of_input,
    parse_error
};

const char *token_type_name(token_type type) {
    switch (type) {
        case token_type::keyword_int:
            return "int";
        case token_type::keyword_float:
            return "float";
        case token_type::keyword_boolean:
            return "boolean";
        case token_type::keyword_function:
            return "function";
        case token_type::keyword_for:
            return "for";
        case token_type::keyword_while:
            return "while";
        case token_type::keyword_if:
            return "if";
        case token_type::keyword_else:
            return "else";
        case token_type::keyword_return:
            return "return";
        case token_type::keyword_break:
            return "break";
        case token_type::keyword_continue:
            return "continue";
        case token_type::literal_true:
            return "true";
        case token_type::literal_false:
            return "false";
        case token_type::literal_int:
            return "literal int";
        case token_type::literal_float:
            return "literal float";
        case token_type::plus:
            return "+";
        case token_type::minus:
             return "-";
        case token_type::asterisk:
             return "*";
        case token_type::slash:
             return "/";
        case token_type::mod:
             return "%";
        case token_type::amp:
            return "&";
        case token_type::logical_and:
            return "&&";
        case token_type::vertical_bar:
             return "|";
        case token_type::colon:
            return ":";
        case token_type::comma:
            return ",";
        case token_type::exclamation:
            return "!";
        case token_type::logical_or:
            return "||";
        case token_type::single_quote:
            return "'";
        case token_type::double_quote:
            return "\"";
        case token_type::semicolon:
            return ";";
        case token_type::assign:
            return "=";
        case token_type::equal:
            return "==";
        case token_type::not_equal:
            return "!=";
        case token_type::less:
            return "<";
        case token_type::greater:
            return ">";
        case token_type::left_parenthese:
            return "(";
        case token_type::right_parenthese:
            return ")";
        case token_type::left_bracket:
            return "[";
        case token_type::right_bracket:
            return "]";
        case token_type::left_brace:
            return "{";
        case token_type::right_brace:
            return "}";
        case token_type::identifier:
            return "identifier";
        case token_type::end_of_input:
            return "<end>";
        case token_type::parse_error:
            return "<parse error>";
        default:
            return "<unknown token>";
    }
}

struct token {
    std::string_view content;
    token_type type;
    std::size_t line;
    std::size_t column;

    token() = default;

    token(token_type t, const position_t &p)
        : type(t), line(p.lines_read + 1),
          column(p.chars_read_current_line) {}

    token(std::string_view c, token_type t, const position_t &p)
        : content(c), type(t),
          line(p.lines_read + 1),
          column(p.chars_read_current_line) {}
};



template <typename InputAdapterType>
class lexer {
 public:
    using char_type = typename InputAdapterType::char_type;
    using char_int_type = typename std::char_traits<char_type>::int_type;

    explicit lexer(InputAdapterType &&adapter)
        : ia_(std::move(adapter)) {}

    position_t position() const {
        return position_;
    }

    void rewind() {
        ia_.rewind();
    }

    token next_token() {
        skip_whitespace();
        
        switch (current_) {
            case '+':
                return token{"+", token_type::plus, position_};
            case '-':
                return token{"-", token_type::minus, position_};
            case '*':
                return token{"*", token_type::asterisk, position_};
            case '/':
                return token{"/", token_type::slash, position_};
            case '%':
                return token{"%", token_type::mod, position_};
            case '&':
                if (get() == '&')
                    return token{"&&", token_type::logical_and, position_};
                unget();
                return token("&", token_type::amp, position_);
            case '|':
                if (get() == '|')
                    return token{"||", token_type::logical_or, position_};
                unget();
                return token{"|", token_type::vertical_bar, position_};
            case ':':
                return token{":", token_type::colon, position_};
            case ',':
                return token{",", token_type::comma, position_};
            case '!':
                if (get() == '=')
                    return token{"!=", token_type::not_equal, position_};
                unget();
                return token{"!", token_type::exclamation, position_};
            case '\'':
                return token{"'", token_type::single_quote, position_};
            case '"':
                return token{"\"", token_type::double_quote, position_};
            case '\\':
                return token{"\\", token_type::backslash, position_};
            case ';':
                return token{";", token_type::semicolon, position_};
            case '=':
                if (get() == '=')
                    return token("==", token_type::equal, position_);
                unget();
                return token{"=", token_type::assign, position_};
            case '<':
                return token{"<", token_type::less, position_};
            case '>':
                return token{">", token_type::greater, position_};
            case '(':
                return token("(", token_type::left_parenthese, position_);
            case ')':
                return token{")", token_type::right_parenthese, position_};
            case '[':
                return token("[", token_type::left_bracket, position_);
            case ']':
                return token{"]", token_type::right_bracket, position_};
            case '{':
                return token{"{", token_type::left_brace, position_};
            case '}':
                return token{"}", token_type::right_brace, position_};
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return scan_number();
            case '\0':
            case std::char_traits<char_type>::eof():
                return token{"eof", token_type::end_of_input, position_};
            default:
                if (std::isalpha(current_) || current_ == '_') {
                    return scan_identifier();
                }
                throw std::runtime_error("unknow token");
        }
    }

//  private:
 public:

    token scan_identifier() {
        reset();
        while (std::isalnum(current_) || current_ == '_') {
            get();
        }
        unget();
        auto iter = keywords_.find(token_string_);
        if (iter != keywords_.end())
            return token{iter->first, iter->second, position_};
        if (token_string_ == "true")
            return token{token_string_, token_type::literal_true, position_};
        if (token_string_ == "false")
            return token{token_string_, token_type::literal_false, position_};
        return token{token_string_, token_type::identifier, position_};
    }

    token scan_number() {
        reset();
        unget();
        int previous_state = -1;
        int state = 0;
        while (state != -1) {
            get();
            switch (state) {
                case 0:
                    switch (current_) {
                        case '0':
                            // add(current_);
                            previous_state = state;
                            state = 2;
                            break;
                        // case '+':
                        // case '-':
                        //     add(current_);
                        //     previous_state = state;
                        //     state = 1;
                        //     break;
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 1:
                    switch (current_) {
                        case '0':
                            // add(current_);
                            previous_state = state;
                            state = 2;
                            break;
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 2:
                    switch (current_) {
                        case '.':
                            // add(current_);
                            previous_state = state;
                            state = 4;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 3:
                    switch (current_) {
                        case '.':
                            // add(current_);
                            previous_state = state;
                            state = 4;
                            break;
                        case 'e':
                        case 'E':
                            // add(current_);
                            previous_state = state;
                            state = 6;
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            // state = 3;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 4:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 5;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 5:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            // state = 5;
                            break;
                        case 'e':
                        case 'E':
                            // add(current_);
                            previous_state = state;
                            state = 6;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 6:
                    switch (current_) {
                        case '+':
                        case '-':
                            // add(current_);
                            previous_state = state;
                            state = 7;
                            break;
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 7:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            // add(current_);
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                case 8:
                    switch (current_) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            // add(current_);
                            previous_state = state;
                            state = 8;
                            break;
                        default:
                            previous_state = state;
                            state = -1;
                            break;
                    }
                    break;
                default:
                    throw std::runtime_error("invaild state");
            }
        }
        // check invaid number such as 123a
        if (std::isalpha(current_)) {
            return token{token_string_, token_type::parse_error, position_};
        }
        unget();
        if (previous_state == 2 || previous_state == 3) {
            return token(token_string_, token_type::literal_int, position_);
        }
        if (previous_state == 5 || previous_state == 8) {
            return token(token_string_, token_type::literal_float, position_);
        }
        return token("invalid number literal", token_type::parse_error, position_);
    }

    token scan_literal(const char *literal, token_type type) {
        assert(literal[0] == std::char_traits<char_type>::to_char_type(current_));
        std::size_t length = 1;
        for (std::size_t i = 1; literal[i]; i++, length++) {
            if (std::char_traits<char_type>::to_char_type(get()) != literal[i]) [[unlikely]] {
                error_message_ = "invalid literal";
                return token{token_type::parse_error, position_};
            }
        }
        return token{std::string_view{literal, length}, type,position_};
    }

    char_int_type get() {
        ++position_.chars_read_total;
        ++position_.chars_read_current_line;

        if (next_unget_) {
            next_unget_ = false;
        } else {
            current_ = ia_.get_character();
        }

        if (current_ != std::char_traits<char_type>::eof()) [[likely]] {
            token_string_.push_back(std::char_traits<char_type>::to_char_type(current_));
        }

        if (current_ == '\n') {
            ++position_.lines_read;
            position_.chars_read_current_line = 0;
        }

        return current_;
    }

    void unget() {
        next_unget_ = true;

        position_.chars_read_total--;
        if (position_.chars_read_current_line == 0) {
            if (position_.lines_read > 0) {
                position_.lines_read--;
            }
        } else {
            position_.chars_read_current_line--;
        }

        if (current_ != std::char_traits<char_type>::eof()) [[likely]] {
            assert(!token_string_.empty());
            token_string_.pop_back();
        }
    }

    void reset() {
        token_string_.clear();
        token_string_.push_back(std::char_traits<char_type>::to_char_type(current_));
    }

    void add(char_int_type c) {
        token_string_.push_back(static_cast<std::string::value_type>(c));
    }

    void skip_whitespace() {
        do {
            get();
        } while (current_ == ' ' || current_ == '\n' || current_ == '\r' || current_ == '\t');
    }

    position_t position_;
    InputAdapterType ia_;
    bool next_unget_ = false;
    char_int_type current_ = std::char_traits<char_type>::eof();
    std::string token_string_;
    const char *error_message_ = "";
    inline static std::unordered_map<std::string_view, token_type> keywords_ = {
        {"if", token_type::keyword_if}, {"else", token_type::keyword_else},
        {"for", token_type::keyword_for}, {"while", token_type::keyword_while},
        {"return", token_type::keyword_return}, {"function", token_type::keyword_function},
        {"int", token_type::keyword_int}, {"boolean", token_type::keyword_boolean},
        {"float", token_type::keyword_float},
        {"break", token_type::keyword_break}, {"continue", token_type::keyword_continue}
    };
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

template <>
struct std::formatter<neroll::script::detail::token_type> {
    constexpr auto parse(std::format_parse_context &context) {
        if (context.begin() != context.end())
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(neroll::script::detail::token_type type, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        return std::format_to(context.out(), "{}", token_type_name(type));
    }
};

template <>
struct std::formatter<neroll::script::detail::token> {
    constexpr auto parse(std::format_parse_context &context) {
        if (context.begin() != context.end())
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(const neroll::script::detail::token &token, std::format_context &context) const {
        using neroll::script::detail::token_type_name;
        using neroll::script::detail::token_type;

        if (token.type == token_type::end_of_input)
            return std::format_to(context.out(), "<EOF, {}>", token_type_name(token.type));
        if (token.type == token_type::parse_error)
            return std::format_to(context.out(), "<parse error, {}>", token_type_name(token.type));
        return std::format_to(context.out(), "<{}, {}>", token.content, token_type_name(token.type));
    }
};

#endif