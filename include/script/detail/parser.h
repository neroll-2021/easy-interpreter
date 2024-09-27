#ifndef NEROLL_SCRIPT_DETAIL_PARSER_H
#define NEROLL_SCRIPT_DETAIL_PARSER_H

#include <memory>   // std::shared_ptr
#include <stack>    // std::stack

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

// template <typename InputAdapterType>
class parser {
 public:
    parser(lexer<file_input_adapter> &&lex)
        : lexer_(std::move(lex)), buffer_(look_ahead_count) {
        for (std::size_t i = 0; i < buffer_.capacity(); i++) {
            get_token();
        }
    }


 public:
//  private:


    // expression_node *parse_term() {

    // }

    // expression_node *parse_expression() {
        
    // }

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

    token current_token() const {
        return buffer_.get_next(0);
    }
    token next_token(std::size_t k) const {
        return buffer_.get_next(k);
    }
    void get_token() {
        buffer_.add(lexer_.next_token());
    }

 private:
    // lexer<InputAdapterType> lexer_;
    lexer<file_input_adapter> lexer_;
    ring_buffer<token> buffer_;

    constexpr static std::size_t look_ahead_count = 2;
};

}

}

}

#endif