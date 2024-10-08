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
#include "script/detail/function.h"
#include "script/detail/exception.h"

namespace neroll {

namespace script {

namespace detail {


template <typename InputAdaptor>
class parser {
 public:
    parser(lexer<input_stream_adapter> &&lex)
        : lexer_(std::move(lex)), buffer_(look_ahead_count) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }

    statement_node *parse() {
        return parse_program();
    }

 private:
    template <typename... Args>
    [[noreturn]]
    void throw_syntax_error_with_location(const std::format_string<Args...> format_string, Args&&... args) {
        throw_syntax_error(
            "line {}, column {}: {}", current_token().line, current_token().column,
            std::format(format_string, std::forward<Args>(args)...)
        );
    }

    template <typename... Args>
    [[noreturn]]
    void throw_execute_error_with_location(const std::format_string<Args...> format_string, Args&&... args) {
        throw_execute_error(
            "line {}, column {}: {}", current_token().line, current_token().column,
            std::format(format_string, std::forward<Args>(args)...)
        );
    }

    template <typename... Args>
    [[noreturn]]
    void throw_symbol_error_with_location(const std::format_string<Args...> format_string, Args&&... args) {
        throw_symbol_error(
            "line {}, column {}: {}", current_token().line, current_token().column,
            std::format(format_string, std::forward<Args>(args)...)
        );
    }

    template <typename... Args>
    [[noreturn]]
    void throw_type_error_with_location(const std::format_string<Args...> format_string, Args&&... args) {
        throw_type_error(
            "line {}, column {}: {}", current_token().line, current_token().column,
            std::format(format_string, std::forward<Args>(args)...)
        );
    }

    statement_node *parse_program() {
        static_symbol_table.push_scope();
        statement_node *node = parse_items();
        static_symbol_table.pop_scope();
        return node;
    }

    statement_node *parse_items() {
        
        block_node *block = new block_node();
        while (current_token_type() != token_type::right_brace && current_token_type() != token_type::end_of_input) {
            statement_node *item = parse_item();
            block->insert(item);
        }
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
                func_call_node *func = dynamic_cast<func_call_node *>(expr);
                return new declaration_node(var_type, var_name, expr);
            } else if (current_token_type() == token_type::semicolon) {
                match(token_type::semicolon);
                return new declaration_node(var_type, var_name, nullptr);
            } else {
                throw_syntax_error_with_location(
                    "expect a ';'"
                );
            }
        } else if (current_token_type() == token_type::keyword_function) {
            static_symbol_table.push_scope();
            statement_node *func = parse_func_decl();
            static_symbol_table.pop_scope();
            return func;
        } else {
            throw_syntax_error_with_location(
                "invalid declaration"
            );
        }
    }
    
    statement_node *parse_func_decl() {
        match(token_type::keyword_function);
        std::string name{current_token().content};
        match(token_type::identifier);
        match(token_type::left_parenthese);
        // parse_param_list
        std::vector<declaration_node *> params = parse_param_list();

        match(token_type::right_parenthese);
        match(token_type::colon);
        
        if (!is_basic_type(current_token_type())) {
            throw_type_error_with_location(
                "function must return a type of int, float or boolean"
            );
        }

        variable_type return_type;

        if (current_token_type() == token_type::keyword_int) {
            return_type = variable_type::integer;
            match(token_type::keyword_int);
        } else if (current_token_type() == token_type::keyword_float) {
            return_type = variable_type::floating;
            match(token_type::keyword_float);
        } else {
            return_type = variable_type::boolean;
            match(token_type::keyword_boolean);
        }

        statement_node *body = parse_block();

        func_decl_node *node = new func_decl_node(return_type, name, body);

        for (const auto p : params) {
            node->add_param(p);
        }
        static_func_decls.add(name, node);
        return node;
    }

    std::vector<declaration_node *> parse_param_list() {
        std::vector<declaration_node *> result;
        int index = 0;
        while (current_token_type() != token_type::right_parenthese && current_token_type() != token_type::end_of_input) {
            if (index != 0) {
                match(token_type::comma);
            }
            if (!is_basic_type(current_token_type())) {
                throw_type_error_with_location("parameter type must be a basic type");
            }
            declaration_node *node = parse_param();
            result.push_back(node);
            index++;
        }
        return result;
    }

    declaration_node *parse_param() {
        variable_type type;
        
        if (current_token_type() == token_type::keyword_int) {
            match(token_type::keyword_int);
            type = variable_type::integer;
        } else if (current_token_type() == token_type::keyword_float) {
            match(token_type::keyword_float);
            type = variable_type::floating;
        } else {
            match(token_type::keyword_boolean);
            type = variable_type::boolean;
        }
        std::string name{current_token().content};
        match(token_type::identifier);
        declaration_node *node = new declaration_node(type, name, nullptr);
        return node;
    }

    bool is_basic_type(token_type type) const {
        return type == token_type::keyword_int || type == token_type::keyword_float ||
               type == token_type::keyword_boolean;
    }

    statement_node *parse_block() {
        match(token_type::left_brace);
        static_symbol_table.push_scope();
        statement_node *items = parse_items();
        static_symbol_table.pop_scope();
        match(token_type::right_brace);
        return items;
    }

    statement_node *parse_statement() {
        if (current_token_type() == token_type::left_brace) {
            // parse_block
            return parse_block();
        } else if (is_iter_keyword(current_token_type())) {
            // parse_iter_statement
            return parse_iter_statement();
        } else if (is_jump_keyword(current_token_type())) {
            // parse_jump statement
            return parse_jump_statement();
        } else if (is_select_keyword(current_token_type())) {
            return parse_select_statement();
        } else {
            return parse_expr_statement();
        }
    }

    bool is_select_keyword(token_type type) const {
        return type == token_type::keyword_if;
    }

    statement_node *parse_select_statement() {
        match(token_type::keyword_if);
        match(token_type::left_parenthese);
        expression_node *condition = parse_expr();
        match(token_type::right_parenthese);
        statement_node *body = parse_statement();
        if_node *node = new if_node(condition, body);
        if (current_token_type() == token_type::keyword_else) {
            match(token_type::keyword_else);
            node->set_else(std::shared_ptr<statement_node>{parse_statement()});
        }
        return node;
    }

    statement_node *parse_jump_statement() {
        if (current_token_type() == token_type::keyword_continue) {
            match(token_type::keyword_continue);
            match(token_type::semicolon);
            return new continue_node;
        } else if (current_token_type() == token_type::keyword_break) {
            match(token_type::keyword_break);
            match(token_type::keyword_break);
            return new break_node;
        } else {
            match(token_type::keyword_return);
            if (current_token_type() == token_type::semicolon) {
                return new return_node(nullptr);
            } else {
                expression_node *expr = parse_expr();
                match(token_type::semicolon);
                return new return_node(expr);
            }
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
            throw_syntax_error_with_location(
                "invalid iteration keyword {}", current_token().content
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
            auto result = static_symbol_table.find(var_name);
            if (!result.has_value()) {
                throw_symbol_error_with_location(
                    "{} is not defined", var_name
                );
            }
            auto [n, t] = result.value();
            match(token_type::identifier);
            match(token_type::assign);
            expression_node *rhs = parse_assign_expr();
            expression_node *var_node = nullptr;
            var_node = new variable_node(n, t);
            expression_node *assign = new assign_node(var_node, rhs);
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

                    if (var_name == "input") {
                        
                        match(token_type::left_parenthese);
                        if (current_token_type() == token_type::keyword_int) {
                            match(token_type::keyword_int);
                            match(token_type::right_parenthese);
                            return new func_call_node(var_name, std::vector<expression_node *>{new int_node(0)});
                        } else if (current_token_type() == token_type::keyword_float) {
                            match(token_type::keyword_float);
                            match(token_type::right_parenthese);
                            return new func_call_node(var_name, std::vector<expression_node *>{new float_node(0.0)});
                        } else if (current_token_type() == token_type::keyword_boolean) {
                            match(token_type::keyword_boolean);
                            match(token_type::right_parenthese);
                            return new func_call_node(var_name, std::vector<expression_node *>{new boolean_node(false)});
                        } else {
                            throw_type_error(
                                "expect a type of int, float or boolean"
                            );
                        }
                    }

                    // parse_func_call
                    match(token_type::left_parenthese);
                    std::vector<expression_node *> args = parse_arg_list();
                    match(token_type::right_parenthese);

                    if (var_name == "println") {
                        if (args.size() != 1) {
                            throw_type_error(
                                "line {} column {}: (function {}) no match arguments",
                                line, col, var_name
                            );
                        }

                        variable_type arg_type = args[0]->value_type();
                        if (arg_type != variable_type::integer && arg_type != variable_type::floating &&
                            arg_type != variable_type::boolean) {
                            throw_type_error(
                                "line {} column {}: (function {}) no match arguments",
                                line, col, var_name
                            );
                        }
                        return new func_call_node(var_name, args);
                    }

                    func_decl_node *func = static_func_decls.find(var_name);
                    if (func == nullptr) {
                        throw_symbol_error(
                            "line {} column {}: function {} is not defined", line, col, var_name
                        );
                    }

                    if (!arguments_match_declaration(func, args)) {
                        throw_type_error(
                            "line {} column {}: (function {}) no match arguments",
                            line, col, var_name
                        );
                    }
                    return new func_call_node(var_name, args);
                } else {
                    auto var = find_variable(var_name);
                    if (!var.has_value()) {
                        throw_symbol_error(
                            "line {} column {}: {} is not defined", line, col, var_name
                        );
                    }
                    auto [n, t] = var.value();
                    switch (t) {
                        case variable_type::integer: {
                            return new variable_node(n, t);
                        }
                        case variable_type::floating: {
                            return new variable_node(n, t);
                        }
                        case variable_type::boolean: {
                            return new variable_node(n, t);
                        }
                        default:
                            throw_type_error_with_location(
                                "invalid variable type {} in expression", t
                            );
                    }
                }
            }
            break;
            default:
                throw_type_error_with_location(
                    "invalid operand {}", current_token().content
                );
        }
    }

    bool arguments_match_declaration(func_decl_node *func, const std::vector<expression_node *> &args) {
        if (func->params().size() != args.size())
            return false;
        const auto &params = func->params();
        for (std::size_t i = 0; i < args.size(); i++) {
            assert(params[i]->value_type() != variable_type::error);
            assert(args[i]->value_type() != variable_type::error);
            if (arithmetic_type_cast(params[i]->value_type(), args[i]->value_type()) == variable_type::error) {
                return false;
            }
        }
        return true;
    }

    std::vector<expression_node *> parse_arg_list() {
        std::vector<expression_node *> result;
        int index = 0;
        while (current_token_type() != token_type::right_parenthese && current_token_type() != token_type::end_of_input) {
            if (index != 0) {
                match(token_type::comma);
            }
            expression_node *expr = parse_expr();
            result.push_back(expr);
            index++;
        }
        return result;
    }

    bool is_function_call() const {
        return current_token_type() == token_type::identifier &&
               next_token(1).type == token_type::left_parenthese;
    }

    bool is_variable_declared(std::string_view variable_name) const {
        return program_scope.current_scope().contains(variable_name);
    }

    std::optional<std::pair<std::string, variable_type>> find_variable(std::string_view variable_name) const {
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

    void match(token_type expect) {
        if (current_token_type() == expect) {
            get_token();
        } else {
            throw_syntax_error(
                "expect '{}', found '{}'", expect, current_token_type()
            );
        }
    }

    lexer<InputAdaptor> lexer_;
    ring_buffer<token> buffer_;

    constexpr static std::size_t look_ahead_count = 2;
};

}

}

}

#endif