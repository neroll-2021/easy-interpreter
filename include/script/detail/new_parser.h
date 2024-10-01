#ifndef NEROLL_SCRIPT_DETAIL_NEW_PARSER_H
#define NEROLL_SCRIPT_DETAIL_NEW_PARSER_H

#include <string_view>
#include <charconv>
#include <stdexcept>    // std::runtime_error
#include <format>       // std::format

#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"
#include "script/detail/ring_buffer.h"
#include "script/detail/ast.h"

namespace neroll {

namespace script {

namespace detail {

class parser {
 public:
    parser(lexer<input_stream_adapter> &&lex)
        : lexer_(std::move(lex)), buffer_(look_ahead_count) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }

//  private:

    expression_node *parse_expr() {
        throw std::runtime_error("parse_expr is not complete yet");
        return nullptr;
    }

    expression_node *parse_equal() {
        expression_node *expr1 = parse_add();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::equal || current_token_type() == token_type::not_equal) {
            if (current_token_type() == token_type::equal) {
                match(token_type::equal);
                expr2 = parse_add();
                op = new equal_node(expr1, expr2);
            } else {
                match(token_type::not_equal);
                expr2 = parse_add();
                op = new not_equal_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_relation() {
        expression_node *expr1 = parse_add();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::less || current_token_type() == token_type::greater) {
            if (current_token_type() == token_type::less) {
                match(token_type::less);
                expr2 = parse_add();
                op = new less_node(expr1, expr2);
            } else {
                match(token_type::greater);
                expr2 = parse_add();
                op = new greater_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_add() {
        expression_node *expr1 = parse_mul();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::plus || current_token_type() == token_type::minus) {
            if (current_token_type() == token_type::plus) {
                match(token_type::plus);
                expr2 = parse_mul();
                op = new add_node(expr1, expr2);
            } else {
                match(token_type::minus);
                expr2 = parse_mul();
                op = new minus_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_mul() {
        expression_node *expr1 = parse_unary();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::asterisk || current_token().type == token_type::slash ||
               current_token_type() == token_type::mod) {
            if (current_token_type() == token_type::asterisk) {
                match(token_type::asterisk);
                expr2 = parse_unary();
                op = new multiply_node(expr1, expr2);
            } else if (current_token_type() == token_type::slash) {
                match(token_type::slash);
                expr2 = parse_unary();
                op = new divide_node(expr1, expr2);
            } else {
                match(token_type::mod);
                expr2 = parse_unary();
                op = new modulus_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_unary() {
        switch (current_token_type()) {
            case token_type::plus:
                match(token_type::plus);
                return parse_unary();
            case token_type::minus: {
                match(token_type::minus);
                expression_node *value = parse_unary();
                return new negative_node(value);
            }
            default:
                return parse_primary();
        }
    }

    expression_node *parse_primary() {
        switch (current_token_type()) {
            case token_type::literal_int:
                return make_node_and_match<int32_t>(token_type::literal_int);
            case token_type::literal_float:
                return make_node_and_match<float>(token_type::literal_float);
            case token_type::literal_true:
                return make_node_and_match<bool>(token_type::literal_true);
            case token_type::literal_false:
                return make_node_and_match<bool>(token_type::literal_false);
            case token_type::left_parenthese: {
                match(token_type::left_parenthese);
                expression_node *node = parse_expr();
                match(token_type::right_parenthese);
                return node;
            }
            case token_type::identifier: {
                std::string var_name{current_token().content};
                auto line = current_token().line;
                auto col = current_token().column;
                match(token_type::identifier);

                if (current_token_type() == token_type::left_parenthese) {
                    throw std::runtime_error("function call is not supported yet");
                } else {
                    if (is_variable_declared(var_name)) {
                        std::shared_ptr<variable> var = find_variable(var_name);
                        switch (var->type()) {
                            case variable_type::integer: {
                                auto p = std::dynamic_pointer_cast<variable_int>(var);
                                return new int_node(p->value());
                            }
                            case variable_type::floating: {
                                auto p = std::dynamic_pointer_cast<variable_float>(var);
                                return new float_node(p->value());
                            }
                            case variable_type::boolean: {
                                auto p = std::dynamic_pointer_cast<variable_boolean>(var);
                                return new boolean_node(p->value());
                            }
                            default:
                                throw std::runtime_error(
                                    std::format("invalid variable type {} in expression",
                                        variable_type_name(var->type()))
                                );
                        }
                    } else {
                        throw std::runtime_error(
                            std::format("line {} column {}: {} is not defined",
                                line, col, var_name)
                        );
                    }
                }
            }
            break;
            default:
                throw std::runtime_error("invalid operand");
        }
    }

    bool is_function_call() const {
        return current_token_type() == token_type::identifier &&
               next_token(1).type == token_type::left_parenthese;
    }

    bool is_variable_declared(std::string_view variable_name) const {
        return program_scope.current_scope().contains(variable_name);
    }

    std::shared_ptr<variable> find_variable(std::string_view variable_name) const {
        return program_scope.current_scope().find(variable_name);
    }

    template <typename T>
    expression_node *make_node_and_match(token_type type) {
        static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, bool>);
        expression_node *node = make_value_node<T>(current_token());
        match(type);
        return node;
    }

    template <typename T>
    expression_node *make_value_node(const token &t) const {
        static_assert(std::is_same_v<T, int32_t> || std::is_same_v<T, float> || std::is_same_v<T, bool>);
        if constexpr (std::is_same_v<T, int32_t>) {
            T value;
            std::string_view literal{t.content};
            std::from_chars(literal.data(), literal.data() + literal.size(), value);
            return new int_node(value);
        } else if constexpr (std::is_same_v<T, float>) {
            T value;
            std::string_view literal{t.content};
            std::from_chars(literal.data(), literal.data() + literal.size(), value);
            return new float_node(value);
        } else {
            if (t.type == token_type::literal_true)
                return new boolean_node(true);
            else
                return new boolean_node(false);
        }
    }

    void get_token() {
        auto t = lexer_.next_token();
        std::println("{}", t);
        buffer_.add(t);
    }

    const token &current_token() const {
        return buffer_.get_next(0);
    }

    token_type current_token_type() const {
        return buffer_.get_next(0).type;
    }

    token next_token(std::size_t k) const {
        return buffer_.get_next(k);
    }

    void match(token_type expected_type) {
        if (current_token().type == expected_type) {
            get_token();
        } else {
            throw std::runtime_error(
                std::format("line {}, column {}: expect {}, found {}",
                    current_token().line, current_token().column,
                    token_type_name(expected_type), token_type_name(current_token().type)
                )
            );
        }
    }

    lexer<input_stream_adapter> lexer_;
    ring_buffer<token> buffer_;

    constexpr static std::size_t look_ahead_count = 2;
};

}

}

}

#endif