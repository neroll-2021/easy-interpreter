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
#include "script/detail/static_symbols.h"

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

    statement_node *parse_program() {
        static_symbol_table.push_scope();
        statement_node *node = parse_items();
        static_symbol_table.pop_scope();
        return node;
    }

    // statement_node *parse_block() {
    //     match(token_type::left_brace);
    //     // block_node *block = new block_node();
    //     // while (current_token_type() != token_type::right_brace) {
    //     //     statement_node *item = parse_item();
    //     //     block->insert(item);
    //     // }
    //     static_symbol_table.push_scope();
    //     statement_node *block = parse_items();
    //     assert(!static_symbol_table.empty());
    //     static_symbol_table.pop_scope();
    //     match(token_type::right_brace);
    //     return block;
    // }

    statement_node *parse_items() {
        
        block_node *block = new block_node();
        while (current_token_type() != token_type::right_brace && current_token_type() != token_type::end_of_input) {
            statement_node *item = parse_item();
            block->insert(item);
        }
        // static_symbol_table.pop_scope();
        return block;
    }

    statement_node *parse_item() {
        if (is_basic_type(current_token_type()) || current_token_type() == token_type::keyword_function) {
            return parse_declaration();
        } else {
            return parse_statement();
        }
    }

    statement_node *parse_declaration() {
        if (is_basic_type(current_token_type())) {
            variable_type var_type;
            if (current_token_type() == token_type::keyword_int) {
                var_type = variable_type::integer;
                match(token_type::keyword_int);
            } else if (current_token_type() == token_type::keyword_float) {
                var_type = variable_type::floating;
                match(token_type::keyword_float);
            } else {
                var_type = variable_type::boolean;
                match(token_type::keyword_boolean);
            }
            std::string var_name{current_token().content};
            match(token_type::identifier);
            if (current_token_type() == token_type::assign) {
                match(token_type::assign);
                expression_node *expr = parse_assign_expr();
                match(token_type::semicolon);
                return new declaration_node(var_type, var_name, expr);
            } else if (current_token_type() == token_type::semicolon) {
                match(token_type::semicolon);
                return new declaration_node(var_type, var_name, nullptr);
            } else {
                throw std::runtime_error(
                    std::format("line {} column {}: expect a ;",
                        current_token().line, current_token().column)
                );
            }
        } else if (current_token_type() == token_type::keyword_function) {
            // todo
            // return parse_func_decl();
            throw std::runtime_error("function declaration is not supported yet");
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: invalid declaration",
                    current_token().line, current_token().column)
            );
        }
    }
    statement_node *parse_func_decl() {
        match(token_type::keyword_function);
        std::string name{current_token().content};
        match(token_type::identifier);
        match(token_type::left_parenthese);
        
        
        return nullptr;
    }

    bool is_basic_type(token_type type) const {
        return type == token_type::keyword_int || type == token_type::keyword_float ||
               type == token_type::keyword_boolean;
    }

    statement_node *parse_statement() {
        if (current_token_type() == token_type::left_brace) {
            // parse_block
            match(token_type::left_brace);
            static_symbol_table.push_scope();
            statement_node *items = parse_items();
            static_symbol_table.pop_scope();
            match(token_type::right_brace);
            return items;
        } else if (is_iter_keyword(current_token_type())) {
            // parse_iter_statement
            return parse_iter_statement();
        } else if (is_jump_keyword(current_token_type())) {
            // parse_jump statement
            return nullptr;
        } else {
            return parse_expr_statement();
        }
    }

    statement_node *parse_iter_statement() {
        if (current_token_type() == token_type::keyword_for) {
            match(token_type::keyword_for);
            match(token_type::left_parenthese);
            expr_statement_node *init = parse_expr_statement();
            expr_statement_node *condition = parse_expr_statement();
            expression_node *update = parse_expr();
            match(token_type::right_parenthese);
            statement_node *body = parse_statement();
            for_node *node = new for_node(init, condition, update, body);
            return node;
        } else if (current_token_type() == token_type::keyword_while) {
            match(token_type::keyword_while);
            match(token_type::left_parenthese);
            expression_node *expr = parse_expr();
            match(token_type::right_parenthese);
            statement_node *body = parse_statement();
            while_node *node = new while_node(expr, body);
            return node;
        } else {
            throw std::runtime_error(
                std::format("invalid iter keyword {}", current_token().content)
            );
        }
    }

    bool is_iter_keyword(token_type type) const {
        return type == token_type::keyword_for || type  == token_type::keyword_while;
    }

    bool is_jump_keyword(token_type type) const {
        return type == token_type::keyword_continue || type == token_type::keyword_break ||
               type == token_type::keyword_return;
    }

    expr_statement_node *parse_expr_statement() {
        if (current_token_type() == token_type::semicolon) {
            match(token_type::semicolon);
            return new expr_statement_node(new void_node);
        } else {
            expression_node *expr = parse_expr();
            auto res = new expr_statement_node(expr);
            match(token_type::semicolon);
            return res;
        }
    }

    expression_node *parse_expr() {
        return parse_assign_expr();
    }

    expression_node *parse_assign_expr() {
        if (current_token_type() == token_type::identifier && next_token(1).type == token_type::assign) {
            std::string var_name{current_token().content};
            // auto var = program_scope.current_scope().find(var_name);
            auto result = static_symbol_table.find(var_name);
            if (!result.has_value()) {
                throw std::runtime_error(
                    std::format("line {} column {}: {} is not defined",
                        current_token().line, current_token().column, var_name)
                );
            }
            auto [n, t] = result.value();
            match(token_type::identifier);
            match(token_type::assign);
            expression_node *rhs = parse_assign_expr();
            expression_node *var_node = nullptr;
            // if (var == nullptr) {
            //     std::println("null");
            //     var_node = new variable_node(var_name, variable_type::error);
            // } else {
            //     std::println("type: {}", variable_type_name(var->type()));
            //     var_node = new variable_node(var_name, var->type());
            // }
            var_node = new variable_node(n, t);
            expression_node *assign = new assign_node(var_node, rhs);
            std::println("xxxxx");
            return assign;
        } else {
            return parse_logical_or();
        }
    }

    expression_node *parse_logical_or() {
        expression_node *expr1 = parse_logical_and();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::logical_or) {
            match(token_type::logical_or);
            expr2 = parse_logical_and();
            op = new logical_or_node(expr1, expr2);
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_logical_and() {
        expression_node *expr1 = parse_equal();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::logical_and) {
            match(token_type::logical_and);
            expr2 = parse_equal();
            op = new logical_and_node(expr1, expr2);
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_equal() {
        expression_node *expr1 = parse_relation();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token_type() == token_type::equal || current_token_type() == token_type::not_equal) {
            if (current_token_type() == token_type::equal) {
                match(token_type::equal);
                expr2 = parse_relation();
                op = new equal_node(expr1, expr2);
            } else {
                match(token_type::not_equal);
                expr2 = parse_relation();
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
                    // if (is_variable_declared(var_name)) {
                        auto var = find_variable(var_name);
                        if (!var.has_value()) {
                            throw std::runtime_error(
                                std::format("{} is not defined", var_name)
                            );
                        }
                        auto [n, t] = var.value();
                        switch (t) {
                            case variable_type::integer: {
                                // auto p = std::dynamic_pointer_cast<variable_int>(var);
                                // return new int_node(p->value());
                                return new variable_node(n, t);
                            }
                            case variable_type::floating: {
                                // auto p = std::dynamic_pointer_cast<variable_float>(var);
                                // return new float_node(p->value());
                                return new variable_node(n, t);
                            }
                            case variable_type::boolean: {
                                // auto p = std::dynamic_pointer_cast<variable_boolean>(var);
                                // return new boolean_node(p->value());
                                return new variable_node(n, t);
                            }
                            default:
                                throw std::runtime_error(
                                    std::format("invalid variable type {} in expression",
                                        variable_type_name(t))
                                );
                        }
                    // } else {
                        // throw std::runtime_error(
                        //     std::format("line {} column {}: {} is not defined",
                        //         line, col, var_name)
                        // );
                    // }
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

    std::optional<std::pair<std::string, variable_type>> find_variable(std::string_view variable_name) const {
        // return program_scope.current_scope().find(variable_name);
        return static_symbol_table.find(variable_name);
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