#ifndef NEROLL_SCRIPT_DETAIL_AST_H
#define NEROLL_SCRIPT_DETAIL_AST_H

#include <cstdint>  // int32_t
#include <memory>   // std::unique_ptr
#include <vector>   // std::vector
#include <format>   // std::format

#include "script/detail/variable.h" // variable_type_cast
#include "script/detail/value_t.h"  // value_t

namespace neroll {

namespace script {

namespace detail {

enum class ast_node_type {
    function, node_for, node_if, node_while, integer, floating, boolean, add, binary
};

class ast_node {
 public:
    explicit ast_node(ast_node_type node_type)
        : node_type_(node_type) {}
    
    virtual ~ast_node() = default;

    ast_node_type node_type() const {
        return node_type_;
    }

 private:
    ast_node_type node_type_;
    // token token_;    // todo
};

// every expression must have type of int, float or boolean
class expression_node : public ast_node {
 public:
    expression_node(ast_node_type node_type)
        : ast_node(node_type) {}

    virtual value_t *evaluate() const = 0;

    variable_type value_type() const {
        return value_type_;
    }
    void set_value_type(variable_type value_type) {
        value_type_ = value_type;
    }

    // static variable_type type_of(value_t value) {
    //     return std::visit([](auto &&v){
    //         using T = std::decay_t<decltype(value)>;
    //         if constexpr (std::is_same_v<T, int32_t>)
    //             return variable_type::integer;
    //         else if constexpr (std::is_same_v<T, float>)
    //             return variable_type::floating;
    //         else if constexpr (std::is_same_v<T, bool>)
    //             return variable_type::boolean;
    //         else
    //             return variable_type::error;
    //     }, value);
    // }

 private:
    variable_type value_type_;
};

class statement_node : public ast_node {
 public:
    statement_node(ast_node_type node_type)
        : ast_node(node_type) {}

    virtual void execute() = 0;
};

class binary_node : public expression_node {
 public:
    binary_node(expression_node *lhs, expression_node *rhs)
        : expression_node(ast_node_type::binary), lhs_(lhs), rhs_(rhs) {}

    std::shared_ptr<expression_node> left() const {
        return lhs_;
    }
    std::shared_ptr<expression_node> right() const {
        return rhs_;
    }

 private:
    std::shared_ptr<expression_node> lhs_;
    std::shared_ptr<expression_node> rhs_;
};

// 
variable_type binary_expression_type(variable_type lhs_type, token_type op, variable_type rhs_type) {
    switch (op) {
        case token_type::plus:
        case token_type::minus:
        case token_type::asterisk:
        case token_type::slash:
            return variable_type_cast(lhs_type, rhs_type);
        case token_type::equal:
        case token_type::less:
        case token_type::greater:
            return variable_type::boolean;
        default:
            return variable_type::error;
    }
}

class add_node : public binary_node {
 public:
    add_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, rhs) {
        variable_type left_type = left()->value_type();
        variable_type right_type = right()->value_type();

        auto type = binary_expression_type(left_type, token_type::plus, right_type);
        if (type == variable_type::error) {
            set_value_type(variable_type::error);
        } else {
            set_value_type(type);
        }
    }

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        value_t *lhs_value = left()->evaluate();
        value_t *rhs_value = right()->evaluate();

        if (value_type() == variable_type::integer) {
            auto l = dynamic_cast<int_value *>(lhs_value);
            auto r = dynamic_cast<int_value *>(rhs_value);
            return new int_value(l->value() + r->value());
        } else if (left()->value_type() == variable_type::integer) {
            auto l = dynamic_cast<int_value *>(lhs_value);
            auto r = dynamic_cast<float_value *>(rhs_value);
            return new float_value(l->value() + r->value());
        } else {
            auto l = dynamic_cast<float_value *>(lhs_value);
            auto r = dynamic_cast<float_value *>(rhs_value);
            return new float_value(l->value() + r->value());
        }
    }
};



class int_node : public expression_node {
 public:
    int_node(int32_t value)
        : expression_node(ast_node_type::integer), value_(value) {
        set_value_type(variable_type::integer);
    }

    value_t *evaluate() const override {
        return new int_value(value_);
    }

 private:
    int32_t value_;
};

class float_node : public expression_node{
 public:
    float_node(float value)
        : expression_node(ast_node_type::floating), value_(value) {
        set_value_type(variable_type::floating);
    }

    value_t *evaluate() const override {
        return new float_value(value_);
    }

 private:
    float value_;
};

class boolean_node : public expression_node {
 public:
    boolean_node(bool value)
        : expression_node(ast_node_type::boolean), value_(value) {
        set_value_type(variable_type::boolean);
    }

    value_t *evaluate() const override {
        return new boolean_value(value_);
    }

 private:
    bool value_;
};

class for_node : public statement_node {
 public:
    for_node() : statement_node(ast_node_type::node_for) {}

    void execute() override {
        init_statement_->evaluate();
        while (condition_->evaluate()) {
            for (auto &statement : statements_) {
                statement->execute();
            }
            update_->evaluate();
        }
    }

 private:
    std::shared_ptr<expression_node> init_statement_;
    std::shared_ptr<expression_node> condition_;
    std::shared_ptr<expression_node> update_;
    std::vector<std::unique_ptr<statement_node>> statements_;
};

class while_node : statement_node {
 public:
    while_node() : statement_node(ast_node_type::node_while) {}

    void execute() override {
        while (condition_->evaluate()) {
            for (auto &statement : statements_) {
                statement->execute();
            }
        }
    }
 private:
    std::shared_ptr<expression_node> condition_;
    std::vector<std::shared_ptr<statement_node>> statements_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif