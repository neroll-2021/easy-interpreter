#ifndef NEROLL_SCRIPT_DETAIL_SYNTAX_CHECKER_H
#define NEROLL_SCRIPT_DETAIL_SYNTAX_CHECKER_H

#include <stdexcept>
#include <format>

#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"
#include "script/detail/ring_buffer.h"

namespace neroll {

namespace script {

namespace detail {

/**
 * 
 * program -> block
 * 
 * block -> { items }
 * 
 * items -> item items
 *        | $
 * 
 * item -> declaration
 *       | statement
 * 
 * statement -> expr_statement
 *            | block
 *            | iter_statement
 *            | jump_statement
 * 
 * expr_statement -> ;
 *                 | expression ;
 * 
 * expression -> assign_expr
 * 
 * assign_expr -> logical_or
 *              | IDENTIFIER = assign_expr
 * 
 * logical_or -> logical_and logical_or'
 * 
 * logical_or' -> || logical_and logical_or'
 *              | $
 * 
 * logical_and -> equal logical_and'
 * 
 * logical_and' -> && equal logical_and'
 *               | $
 * 
 * equal -> relation equal'
 * 
 * equal' -> == relation equal'
 *         | $
 * 
 * relation -> add relation'
 * 
 * relation' -> < add relation'
 *            | > add relation'
 *            | $
 * 
 * add -> mul add'
 * 
 * add' -> + mul add'
 *       | - mul add'
 *       | $
 * 
 * mul -> unary mul'
 * 
 * mul' -> * unary mul'
 *       | / unary mul'
 *       | % unary mul'
 *       | $
 * 
 * unary -> + unary
 *        | - unary
 *        | primary
 * 
 * primary -> IDENTIFIER
 *          | IDENTIFIER ( arg_list )
 *          | CONSTANT
 *          | ( expression )
 * 
 * arg_list -> expression args
 *           | $
 * 
 * args -> , expression args
 *       | $
 * 
 * declaration -> type init ;
 *              | func_decl
 * 
 * type -> int | float | boolean
 * 
 * init -> IDENTIFIER
 *       | IDENTIFIER = assign_expr
 * 
 * func_decl -> 'function' IDENTIFIER '(' param_list ')' ':' block 
 * 
 * param_list -> type IDENTIFIER params
 *             | $
 * 
 * params -> , type IDENTIFIER params
 *         | $
 * 
 * iter_statement -> for ( expr_statement expr_statement expression ) statement
 *                 | while (expression ) statement
 * 
 * jump_statement -> continue ;
 *                 | break ;
 *                 | return ;
 *                 | return expression ;
 * 
 */
// template <typename T>
class syntax_checker {
 public:
    syntax_checker(lexer<input_stream_adapter> &&lex)
        : lexer_(std::move(lex)), buffer_(look_ahead_count) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }
    
    void check() {
        check_program();
    }

 private:
    void check_program() {
        check_items();
    }

    void check_block() {
        match(token_type::left_brace);
        check_items();
        match(token_type::right_brace);
    }

    void check_items() {
        if (current_token().type == token_type::right_brace || current_token().type == token_type::end_of_input) {
            return;
        }
        check_item();
        check_items();
    }

    // TODO
    void check_item() {
        if (current_token().type == token_type::keyword_int ||
            current_token().type == token_type::keyword_float ||
            current_token().type == token_type::keyword_boolean ||
            current_token().type == token_type::keyword_function) {
            check_declaration();
        } else {
            check_statement();
        }
        
    }

    void check_statement() {
        if (current_token().type == token_type::left_brace) {
            check_block();
        } else if (current_token().type == token_type::keyword_for ||
                   current_token().type == token_type::keyword_while) {
            check_iter_statement();
        } else if (current_token().type == token_type::keyword_continue ||
                   current_token().type == token_type::keyword_break ||
                   current_token().type == token_type::keyword_return) {
            check_jump_statement();
        } else {
            check_expr_statement();
        }
    }

    void check_expr_statement() {
        if (current_token().type == token_type::semicolon) {
            match(token_type::semicolon);
        } else {
            check_expr();
            match(token_type::semicolon);
        }
    }

    void check_expr() {
        check_assign_expr();
    }
    
    void check_assign_expr() {
        if (current_token().type == token_type::identifier && next_token(1).type == token_type::assign) {
            match(token_type::identifier);
            match(token_type::assign);
            check_assign_expr();
        } else {
            check_logical_or();
        }
    }

    void check_logical_or() {
        check_logical_and();
        check_logical_or1();
    }

    void check_logical_or1() {
        if (current_token().type == token_type::logical_or) {
            check_logical_and();
            check_logical_or1();
        }
    }

    void check_logical_and() {
        check_equal();
        check_logical_and1();
    }

    void check_logical_and1() {
        if (current_token().type == token_type::logical_and) {
            check_equal();
            check_logical_and1();
        }
    }

    void check_equal() {
        check_relation();
        check_equal1();
    }

    void check_equal1() {
        if (current_token().type == token_type::equal) {
            match(token_type::equal);
            check_relation();
            check_equal1();
        }
    }

    void check_relation() {
        check_add();
        check_relation1();
    }

    void check_relation1() {
        if (current_token().type == token_type::less) {
            match(token_type::less);
            check_add();
            check_relation1();
        } else if (current_token().type == token_type::greater) {
            match(token_type::minus);
            check_add();
            check_relation1();
        }
    }

    void check_add() {
        check_mul();
        check_add1();
    }

    void check_add1() {
        if (current_token().type == token_type::plus) {
            match(token_type::plus);
            check_mul();
            check_add1();
        } else if (current_token().type == token_type::minus) {
            match(token_type::minus);
            check_mul();
            check_add1();
        }
    }

    void check_mul() {
        check_unary();
        check_mul1();
    }

    void check_mul1() {
        if (current_token().type == token_type::asterisk) {
            match(token_type::asterisk);
            check_unary();
            check_mul1();
        } else if (current_token().type == token_type::slash) {
            match(token_type::slash);
            check_unary();
            check_mul1();
        } else if (current_token().type == token_type::mod) {
            match(token_type::mod);
            check_unary();
            check_mul1();
        }
    }

    void check_unary() {
        if (current_token().type == token_type::plus) {
            match(token_type::plus);
            check_unary();
        } else if (current_token().type == token_type::minus) {
            match(token_type::minus);
            check_unary();
        } else {
            check_primary();
        }
    }

    void check_primary() {
        switch (current_token().type) {
            case token_type::identifier:
                match(token_type::identifier);
                if (current_token().type == token_type::left_parenthese) {
                    match(token_type::left_parenthese);
                    check_arg_list();
                    match(token_type::right_parenthese);
                    return;
                }
                break;
            case token_type::literal_int:
                match(token_type::literal_int);
                break;
            case token_type::literal_float:
                match(token_type::literal_float);
                break;
            case token_type::literal_true:
                match(token_type::literal_true);
                break;
            case token_type::literal_false:
                match(token_type::literal_false);
                break;
            default:
                throw std::runtime_error(
                    std::format("line {} column {}: unknown token {} when parsing primary (expect an expression)",
                        current_token().line, current_token().column, token_type_name(current_token().type))
                );
        }
    }

    void check_arg_list() {
        if (current_token().type == token_type::right_parenthese)
            return;
        check_expr();
        check_args();
    }

    void check_args() {
        if (current_token().type == token_type::semicolon) {
            match(token_type::semicolon);
            check_expr();
            check_args();
        }
    }

    void check_declaration() {
        if (current_token().type == token_type::keyword_int ||
            current_token().type == token_type::keyword_float ||
            current_token().type == token_type::keyword_boolean) {
            check_type();
            check_init();
        } else if (current_token().type == token_type::keyword_function) {
            check_func_decl();
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: expect a type name",
                    current_token().line, current_token().column)
            );
        }
    }

    void check_init() {
        match(token_type::identifier);
        if (current_token().type == token_type::assign) {
            match(token_type::assign);
            check_assign_expr();
        }
    }

    void check_type() {
        if (current_token().type == token_type::keyword_int) {
            match(token_type::keyword_int);
        } else if (current_token().type == token_type::keyword_float) {
            match(token_type::keyword_float);
        } else if (current_token().type == token_type::keyword_boolean) {
            match(token_type::keyword_boolean);
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: invalid type",
                    current_token().line, current_token().column)
            );
        }
    }

    void check_func_decl() {
        match(token_type::keyword_function);
        match(token_type::identifier);
        match(token_type::left_parenthese);
        check_param_list();
        match(token_type::right_parenthese);
        match(token_type::colon);

        if (current_token().type == token_type::keyword_int) {
            match(token_type::keyword_int);
        } else if (current_token().type == token_type::keyword_float) {
            match(token_type::keyword_float);
        } else if (current_token().type == token_type::keyword_boolean) {
            match(token_type::keyword_boolean);
        } else if (current_token().type == token_type::keyword_function) {
            throw std::runtime_error(
                std::format("line {} column {}: cannot return a function",
                    current_token().line, current_token().column)
            );
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: expect a type name",
                    current_token().line, current_token().column)
            );
        }

        check_block();
    }

    void check_param_list() {
        if (current_token().type == token_type::right_parenthese) {
            return;
        }
        if (current_token().type == token_type::keyword_int) {
            match(token_type::keyword_int);
        } else if (current_token().type == token_type::keyword_float) {
            match(token_type::keyword_float);
        } else if (current_token().type == token_type::keyword_boolean) {
            match(token_type::keyword_boolean);
        } else if (current_token().type == token_type::keyword_function) {
            throw std::runtime_error(
                std::format("line {} column {}: cannot declare a function type variable",
                    current_token().line, current_token().column)
            );
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: expect a type name",
                    current_token().line, current_token().column)
            );
        }

        match(token_type::identifier);
        check_params();
    }

    void check_params() {
        if (current_token().type == token_type::comma) {
            match(token_type::comma);
            if (current_token().type == token_type::keyword_int) {
                match(token_type::keyword_int);
            } else if (current_token().type == token_type::keyword_float) {
                match(token_type::keyword_float);
            } else if (current_token().type == token_type::keyword_boolean) {
                match(token_type::keyword_boolean);
            } else if (current_token().type == token_type::keyword_function) {
                throw std::runtime_error(
                    std::format("line {} column {}: cannot declare a function type variable",
                        current_token().line, current_token().column)
                );
            } else {
                throw std::runtime_error(
                    std::format("line {} column {}: expect a type name",
                        current_token().line, current_token().column)
                );
            }

            match(token_type::identifier);
            check_params();
        }
    }

    void check_iter_statement() {
        if (current_token().type == token_type::keyword_for) {
            check_for();
        } else if (current_token().type == token_type::keyword_while) {
            check_while();
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: invalid loop key word",
                    current_token().line, current_token().column)
            );
        }
    }

    void check_for() {
        match(token_type::keyword_for);
        match(token_type::left_parenthese);
        check_expr_statement();
        check_expr_statement();
        check_expr();
        match(token_type::right_parenthese);
        check_statement();
    }

    void check_while() {
        match(token_type::keyword_while);
        match(token_type::left_parenthese);
        check_expr();
        match(token_type::right_parenthese);
        check_statement();
    }

    void check_jump_statement() {
        if (current_token().type == token_type::keyword_continue) {
            match(token_type::keyword_continue);
            match(token_type::semicolon);
        } else if (current_token().type == token_type::keyword_break) {
            match(token_type::keyword_break);
            match(token_type::semicolon);
        } else if (current_token().type == token_type::keyword_return) {
            match(token_type::keyword_return);
            if (current_token().type == token_type::semicolon) {
                match(token_type::semicolon);
            } else {
                check_expr();
                match(token_type::semicolon);
            }
        } else {
            throw std::runtime_error(
                std::format("line {} column {}: invalid jump key word",
                    current_token().line, current_token().column)
            );
        }
    }

    void get_token() {
        buffer_.add(lexer_.next_token());
    }

    const token &current_token() const {
        return buffer_.get_next(0);
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