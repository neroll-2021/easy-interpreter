#ifndef NEROLL_SCRIPT_DETAIL_PARSER_H
#define NEROLL_SCRIPT_DETAIL_PARSER_H

#include <memory>   // std::shared_ptr
#include <stack>    // std::stack

#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"    // lexer
#include "script/detail/ast.h"      // 

namespace neroll {

namespace script {

namespace detail {

// template <typename InputAdapterType>
class parser {
 
 public:
//  private:

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

 private:
    // lexer<InputAdapterType> lexer_;
    lexer<file_input_adapter> lexer_;
};

}

}

}

#endif