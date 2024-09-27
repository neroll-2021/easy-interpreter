#ifndef NEROLL_SCRIPT_DETAIL_AST_H
#define NEROLL_SCRIPT_DETAIL_AST_H

#include <cstdint>  // int32_t
#include <memory>   // std::unique_ptr
#include <vector>   // std::vector
#include <format>   // std::format

#include "script/detail/variable.h" // variable_type_cast
#include "script/detail/value_t.h"  // value_t
#include "script/detail/operator.h" // plus, minus, multiplies, divides

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

 private:
    variable_type value_type_;
};

class statement_node : public ast_node {
 public:
    statement_node(ast_node_type node_type)
        : ast_node(node_type) {}

    virtual void execute() = 0;
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

class binary_node : public expression_node {
 public:
    binary_node(expression_node *lhs, token_type op, expression_node *rhs)
        : expression_node(ast_node_type::binary), lhs_(lhs), rhs_(rhs) {
        variable_type left_type = left()->value_type();
        variable_type right_type = right()->value_type();

        auto type = binary_expression_type(left_type, op, right_type);
        set_value_type(type);
    }

    std::shared_ptr<expression_node> left() const {
        return lhs_;
    }
    std::shared_ptr<expression_node> right() const {
        return rhs_;
    }

    value_t *select_operator(token_type op) const;

    template <typename Op>
    value_t *evaluate_result() const {
        assert(value_type() != variable_type::error);
        value_t *lhs_value = left()->evaluate();
        value_t *rhs_value = right()->evaluate();

        if (value_type() == variable_type::integer) {
            auto l = dynamic_cast<int_value *>(lhs_value);
            auto r = dynamic_cast<int_value *>(rhs_value);
            return new int_value(Op{}(l->value(), r->value()));
        } else if (left()->value_type() == variable_type::integer) {
            auto l = dynamic_cast<int_value *>(lhs_value);
            auto r = dynamic_cast<float_value *>(rhs_value);
            return new float_value(Op{}(l->value(), r->value()));
        } else if (right()->value_type() == variable_type::integer) {
            auto l = dynamic_cast<float_value *>(lhs_value);
            auto r = dynamic_cast<int_value *>(rhs_value);
            return new float_value(Op{}(l->value(), r->value()));
        } else {
            auto l = dynamic_cast<float_value *>(lhs_value);
            auto r = dynamic_cast<float_value *>(rhs_value);
            return new float_value(Op{}(l->value(), r->value()));
        }
    }

 private:
    std::shared_ptr<expression_node> lhs_;
    std::shared_ptr<expression_node> rhs_;
};

template <>
value_t *binary_node::evaluate_result<modulus>() const {
    assert(left()->value_type() == variable_type::integer);
    assert(right()->value_type() == variable_type::integer);

    value_t *lhs_value = left()->evaluate();
    value_t *rhs_value = right()->evaluate();

    auto lhs = dynamic_cast<int_value *>(lhs_value);
    auto rhs = dynamic_cast<int_value *>(rhs_value);

    return new int_value(lhs->value() % rhs->value());
}

value_t *binary_node::select_operator(token_type op) const {
    switch (op) {
        case token_type::plus:
            return evaluate_result<plus>();
        case token_type::minus:
            return evaluate_result<minus>();
        case token_type::asterisk:
            return evaluate_result<multiplies>();
        case token_type::slash:
            return evaluate_result<divides>();
        case token_type::mod:
            return evaluate_result<modulus>();
        default:
            return new error_value(std::format("invalid operator {}", token_type_name(op)));
    }
}

class add_node : public binary_node {
 public:
    add_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::plus, rhs) {}

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator + between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        return select_operator(token_type::plus);
    }
};

class minus_node : public binary_node {
 public:
    minus_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::minus, rhs) {}

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator - between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        return select_operator(token_type::minus);
    }
};

class multiply_node : public binary_node {
 public:
    multiply_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::asterisk, rhs) {}

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator * between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        return select_operator(token_type::asterisk);
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
        assert(condition_->value_type() == variable_type::boolean);
        init_statement_->evaluate();
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
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
    std::vector<std::shared_ptr<statement_node>> statements_;
};

class while_node : statement_node {
 public:
    while_node() : statement_node(ast_node_type::node_while) {}

    void execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
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