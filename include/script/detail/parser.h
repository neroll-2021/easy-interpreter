#ifndef NEROLL_SCRIPT_DETAIL_PARSER_H
#define NEROLL_SCRIPT_DETAIL_PARSER_H

#include <memory>   // std::shared_ptr
#include <stack>    // std::stack
#include <cmath>    // std::isinf, std::isnan

#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"            // lexer
#include "script/detail/ast.h"              // 
#include "script/detail/ring_buffer.h"      // ring_buffer

namespace neroll {

namespace script {

namespace detail {

/*
 *
 * BNF of script
 * 
 * expr -> term
 *       | term + term
 *       | term - term
 * 
 * term -> factor
 *       | factor * factor
 *       | factor / factor
 *       | factor % factor
 * 
 * factor -> unary
 * 
 * unary -> primary
 *        | + unary
 *        | - unary
 * 
 * primary -> IDENTIFIER
 *          | constant
 *          | ( expr )
 *          | IDENTIFIER ( arg_list )
 * 
 * arg_list -> expr args
 * 
 * args -> , expr args
 *       | <null>
 * 
 * constant -> INT | FLOAT | BOOLEAN
 * 
 * IDENTIFIER -> [a-zA-Z_][a-zA-Z0-9_]*
 * 
 * INT -> 0|[1-9][0-9]*
 * 
 * FLOAT -> 
 * 
 * BOOLEAN -> true | false
 * 
 */

template <typename InputAdapterType>
class parser {
 public:
    parser(lexer<InputAdapterType> &&lex)
        : lexer_(std::move(lex)), buffer_(look_ahead_count) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }


 public:
//  private:

    expression_node *parse_expr() {
        expression_node *expr1 = parse_term();

        if (current_token().type == token_type::plus) {
            match(token_type::plus);
            expression_node *expr2 = parse_term();
            expression_node *node = new add_node(expr1, expr2);
            return node;
        } else if (current_token().type == token_type::minus) {
            match(token_type::minus);
            expression_node *expr2 = parse_term();
            expression_node *node = new minus_node(expr1, expr2);
            return node;
        }
        return expr1;
    }


    expression_node *parse_term() {
        expression_node *expr1 = parse_factor();

        if (current_token().type == token_type::asterisk) {
            match(token_type::asterisk);
            expression_node *expr2 = parse_factor();
            expression_node *node = new multiply_node(expr1, expr2);
            return node;
        } else if (current_token().type == token_type::slash) {
            match(token_type::slash);
            expression_node *expr2 = parse_factor();
            expression_node *node = new divide_node(expr1, expr2);
            return node;
        } else if (current_token().type == token_type::mod) {
            match(token_type::mod);
            expression_node *expr2 = parse_factor();
            expression_node *node = new modulus_node(expr1, expr2);
            return node;
        }
        return expr1;
    }

    expression_node *parse_factor() {
        return parse_unary();
    }

    expression_node *parse_unary() {
        if (current_token().type == token_type::plus) {
            match(token_type::plus);
            return parse_unary();
        } else if (current_token().type == token_type::minus) {
            match(token_type::minus);
            return new negative_node(parse_unary());
        } else {
            return parse_primary();
        }
    }

    expression_node *parse_primary() {
        if (current_token().type == token_type::identifier && next_token(1).type == token_type::left_brace) {
            match(token_type::identifier);
            match(token_type::left_brace);
            throw std::runtime_error("function call is not supported yet");
        } else if (current_token().type == token_type::identifier) {
            match(token_type::identifier);
            throw std::runtime_error("variable is not supported yet");
        } else if (current_token().type == token_type::left_brace) {
            match(token_type::left_brace);
            expression_node *expr = parse_expr();
            match(token_type::right_brace);
            return expr;
        } else if (current_token().type == token_type::literal_int) {
            std::string_view literal = current_token().content;
            int32_t value;
            auto [ptr, err] = std::from_chars(literal.data(), literal.data() + literal.size(), value);
            if (err == std::errc::invalid_argument) {
                throw std::invalid_argument(std::format("parse primary(): invalid integer {}", literal));
            }
            match(token_type::literal_int);
            return new int_node(value);
        } else if (current_token().type == token_type::literal_float) {
            std::string_view literal = current_token().content;
            float value = std::stof(std::string{literal});
            if (std::isnan(value)) {
                throw std::runtime_error("invalid float");
            }
            if (std::isinf(value)) {
                throw std::runtime_error("float too large");
            }
            return new float_node(value);
        } else if (current_token().type == token_type::literal_true) {
            match(token_type::literal_true);
            return new boolean_node(true);
        } else if (current_token().type == token_type::literal_false) {
            match(token_type::literal_false);
            return new boolean_node(false);
        } else {
            throw std::runtime_error("invalid grammar");
        }
    }

    std::shared_ptr<expression_node> build_expression_tree() {
        auto token = lexer_.next_token();
        std::stack<expression_node *> stack;
        std::stack<token_type> ops;
        auto rhs = stack.top();
        stack.pop();
        auto lhs = stack.top();
        stack.pop();
        auto lhs_type = lhs->value_type();
        auto rhs_type = rhs->value_type();
        auto type = variable_type_cast(lhs_type, rhs_type);
        if (type == variable_type::error) {
            // throw ...
        }
        expression_node *node = nullptr;
        auto op = ops.top();
        ops.pop();
        switch (op) {
            case token_type::plus:
                node = new add_node(lhs, rhs);
            default:
                throw std::runtime_error("invalid operand");
        }
        node->set_value_type(type);
        return std::shared_ptr<expression_node>(node);
    }

    void execute(std::shared_ptr<expression_node> node) {
        if (node->node_type() == ast_node_type::add) {
            
        }
    }

    const token &current_token() const {
        return buffer_.get_next(0);
    }
    token next_token(std::size_t k) const {
        return buffer_.get_next(k);
    }
    void get_token() {
        buffer_.add(lexer_.next_token());
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

 private:
    lexer<InputAdapterType> lexer_;
    // lexer<file_input_adapter> lexer_;
    ring_buffer<token> buffer_;

    constexpr static std::size_t look_ahead_count = 2;
};

}

}

}

#endif