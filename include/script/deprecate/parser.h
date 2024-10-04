#ifndef NEROLL_SCRIPT_DETAIL_PARSER_H
#define NEROLL_SCRIPT_DETAIL_PARSER_H

#include <memory>   // std::shared_ptr
#include <stack>    // std::stack
#include <cmath>    // std::isinf, std::isnan

#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"            // lexer
#include "script/detail/ast.h"              // 
#include "script/detail/ring_buffer.h"      // ring_buffer
#include "script/detail/scope.h"            // scope

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

/*
 *
 * declaration -> type IDENTIFIER initiation ;
 * 
 * initiation -> = expr
 *             | $
 * 
 * type -> int | float | bool
 * 
 * block -> statement block
 *        | $
 * 
 * statement -> declaration    (todo)
 *            | expr_statement
 * 
 * assign -> IDENTIFIER = expr_statement
 * 
 * expr_statement -> expr ;
 * 
 * expr -> term expr'
 * 
 * expr' -> + term expr'
 *        | - term expr'
 *        | $
 * 
 * term -> factor term'
 * 
 * term' -> * factor term'
 *        | / factor term'
 *        | % factor term'
 *        | $
 * 
 * factor -> unary
 * 
 * unary -> + unary
 *        | - unary
 *        | primary
 * 
 * primary -> IDENTIFIER
 *          | CONSTANT
 *          | { expr }
 *          | IDENTIFIER ( arg_list )
 *
 * arg_list -> expr args
 *           | $
 * 
 * args -> , expr args
 *       | $
 * 
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

    statement_node *parse_block() {
        block_node *node = new block_node();
        statement_node *statement = parse_statement();
        while (statement != nullptr) {
            node->insert(statement);
            statement = parse_statement();
        }
        return node;
    }

    statement_node *parse_statement() {
        if (current_token().type == token_type::keyword_int ||
            current_token().type == token_type::keyword_float ||
            current_token().type == token_type::keyword_boolean) {
            return parse_declaration();
        }
        if (current_token().type == token_type::identifier && next_token(1).type == token_type::assign) {

        }
        return nullptr;
    }

    

    statement_node *parse_declaration() {
        declaration_node *node = nullptr;
        expression_node *init_value = nullptr;
        std::string var_name;
        variable_type var_type = variable_type::error;
        switch (current_token().type) {
            case token_type::keyword_int:
                match(token_type::keyword_int);
                var_name = current_token().content;
                match(token_type::identifier);
                init_value = parse_initiation();
                var_type = variable_type::integer;
                break;
            case token_type::keyword_float:
                match(token_type::keyword_float);
                var_name = current_token().content;
                match(token_type::identifier);
                init_value = parse_initiation();
                var_type = variable_type::floating;
                break;
            case token_type::keyword_boolean:
                match(token_type::keyword_boolean);
                var_name = current_token().content;
                match(token_type::identifier);
                init_value = parse_initiation();
                var_type = variable_type::boolean;
                break;
            case token_type::identifier:
                throw std::runtime_error("invalid indentifier in variable declaration");
            default:
                // return nullptr;
                throw std::runtime_error("invalid variable type in declaration");
        }
        node = new declaration_node(var_type, var_name, init_value);
        match(token_type::semicolon);
        return node;
    }

    expression_node *parse_initiation() {
        if (current_token().type == token_type::semicolon) {
            return nullptr;
        }
        match(token_type::assign);
        return parse_expr();
    }

    expression_node *parse_expr() {
        expression_node *expr1 = parse_term();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token().type == token_type::plus || current_token().type == token_type::minus) {
            if (current_token().type == token_type::plus) {
                match(token_type::plus);
                expr2 = parse_term();
                op = new add_node(expr1, expr2);
            } else {
                match(token_type::minus);
                expr2 = parse_term();
                op = new minus_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
        }
        return expr1;
    }

    expression_node *parse_term() {
        expression_node *expr1 = parse_factor();
        expression_node *expr2 = nullptr;
        expression_node *op = nullptr;
        while (current_token().type == token_type::asterisk || current_token().type == token_type::slash ||
               current_token().type == token_type::mod) {
            if (current_token().type == token_type::asterisk) {
                match(token_type::asterisk);
                expr2 = parse_factor();
                op = new multiply_node(expr1, expr2);
            } else if (current_token().type == token_type::slash) {
                match(token_type::slash);
                expr2 = parse_factor();
                op = new divide_node(expr1, expr2);
            } else {
                match(token_type::mod);
                expr2 = parse_factor();
                op = new modulus_node(expr1, expr2);
            }
            expr1 = op;
            expr2 = nullptr;
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
            return parse_unary();
        } else {
            return parse_primary();
        }
    }

    expression_node *parse_primary() {
        if (current_token().type == token_type::literal_int) {
            auto n = make_value_node<int32_t>(current_token());
            match(token_type::literal_int);
            return n;
        } else if (current_token().type == token_type::literal_float) {
            auto n = make_value_node<float>(current_token());
            match(token_type::literal_float);
            return n;
        } else if (current_token().type == token_type::left_parenthese) {
            match(token_type::left_parenthese);
            auto n = parse_expr();
            match(token_type::right_parenthese);
            return n;
        } else if (current_token().type == token_type::identifier) {
            std::string name{current_token().content};
            auto line = current_token().line;
            auto col = current_token().column;
            match(token_type::identifier);
            if (current_token().type != token_type::left_parenthese) {
                if (program_scope.current_scope().contains(name)) {
                    auto v = program_scope.current_scope().find(name);
                    if (v->type() == variable_type::integer) {
                        auto p = std::dynamic_pointer_cast<variable_int>(v);
                        return new int_node(p->value());
                    } else if (v->type() == variable_type::floating) {
                        auto p = std::dynamic_pointer_cast<variable_float>(v);
                        return new float_node(p->value());
                    } else if (v->type() == variable_type::boolean) {
                        auto p = std::dynamic_pointer_cast<variable_boolean>(v);
                        return new boolean_node(p->value());
                    } else {
                        throw std::runtime_error(
                            std::format("invalid variable type {} in expression",
                                variable_type_name(v->type()))
                        );
                    }
                } else {
                    throw std::runtime_error(
                        std::format("line {} column {}: {} is not defined",
                            line, col, name)
                    );
                }
            } else {
                throw std::runtime_error("function call is not supported yet");
            }
            // throw std::runtime_error("variable and function are not supported yet");
        } else if (current_token().type == token_type::literal_true) {
            auto n = make_value_node<bool>(current_token());
            match(token_type::literal_true);
            return n;
        } else if (current_token().type == token_type::literal_false) {
            auto n = make_value_node<bool>(current_token());
            match(token_type::literal_false);
            return n;
        } else {
            throw std::runtime_error("invalid operand");
        }
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
        } else if constexpr (std::is_same_v<T, bool>) {
            if (t.type == token_type::literal_true)
                return new boolean_node(true);
            else
                return new boolean_node(false);
        } else {
            throw std::runtime_error("make_value_node(): invalid value type");
        }
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
        auto token = lexer_.next_token();
        std::println("{}", token);
        buffer_.add(token);
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
    ring_buffer<token> buffer_;

    constexpr static std::size_t look_ahead_count = 2;
};

}

}

}

#endif