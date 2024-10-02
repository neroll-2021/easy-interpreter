#ifndef NEROLL_SCRIPT_DETAIL_AST_H
#define NEROLL_SCRIPT_DETAIL_AST_H

#include <cstdint>  // int32_t
#include <memory>   // std::unique_ptr
#include <vector>   // std::vector
#include <format>   // std::format

#include "script/detail/variable.h"     // variable_type_cast
#include "script/detail/value_t.h"      // value_t
#include "script/detail/operator.h"     // plus, minus, multiplies, divides
#include "script/detail/scope.h"        // program_scope
#include "script/detail/static_symbols.h"
#include "script/detail/function.h"

namespace neroll {

namespace script {

namespace detail {

enum class ast_node_type {
    func_decl, node_for, node_if, node_while, integer, floating, boolean, add, binary, unary,
    declaration, block, var_node, expr_statement, empty, jump, func_call
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

    virtual variable_type value_type() const {
        return value_type_;
    }
    virtual void set_value_type(variable_type value_type) {
        value_type_ = value_type;
    }

 private:
    variable_type value_type_;
};

enum class execute_state {
    normal, broken, continued, returned
};

class statement_node : public ast_node {
 public:
    statement_node(ast_node_type node_type)
        : ast_node(node_type) {}
    
    virtual ~statement_node() {}

    virtual std::pair<execute_state, value_t *> execute() = 0;
};

// 
variable_type binary_expression_type(variable_type lhs_type, token_type op, variable_type rhs_type) {
    assert(lhs_type != variable_type::error);
    assert(rhs_type != variable_type::error);
    switch (op) {
        case token_type::plus:
        case token_type::minus:
        case token_type::asterisk:
        case token_type::slash:
            return variable_type_cast(lhs_type, rhs_type);
        case token_type::mod:
            if (lhs_type != variable_type::integer || rhs_type != variable_type::integer)
                return variable_type::error;
            return variable_type::integer;
        case token_type::equal:
        case token_type::not_equal:
        case token_type::less:
        case token_type::greater:
        case token_type::logical_and:
        case token_type::logical_or:
            return variable_type::boolean;
        case token_type::assign:
            return lhs_type;
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

        assert(left_type != variable_type::error);
        assert(right_type != variable_type::error);
        auto type = binary_expression_type(left_type, op, right_type);
        assert(type != variable_type::error);

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
        if constexpr (std::is_same_v<Op, plus>) {
            std::println("evaluate_result");
        }
        assert(value_type() != variable_type::error);
        
        assert(left() != nullptr);
        assert(right() != nullptr);

        // @bug 
        value_t *lhs_value = left()->evaluate();
        value_t *rhs_value = right()->evaluate();

        std::println("value type: {}", variable_type_name(value_type()));

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
    std::println("select_operator");
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

class unary_node : public expression_node {
 public:
    unary_node(expression_node *value)
        : expression_node(ast_node_type::unary), value_(value) {}

    std::shared_ptr<expression_node> value() const {
        return value_;
    }

 private:
    std::shared_ptr<expression_node> value_;
};

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

        std::println("add evaluate");

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

class divide_node : public binary_node {
 public:
    divide_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::slash, rhs) {}

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator / between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        return select_operator(token_type::slash);
    }
};

class modulus_node : public binary_node {
 public:
    modulus_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::mod, rhs) {}

    virtual value_t *evaluate() const override {
        if (value_type() == variable_type::error) {
            return new error_value(
                std::format("invalid operator % between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        return select_operator(token_type::mod);
    }
};

bool is_relation_oeprator(token_type type) {
    return type == token_type::less || type == token_type::greater ||
           type == token_type::equal || type == token_type::not_equal;
}

bool can_compare(variable_type lhs, token_type op, variable_type rhs) {
    assert(is_relation_oeprator(op));
    if (op == token_type::less || op == token_type::greater) {
        if (lhs == variable_type::boolean || rhs == variable_type::boolean) {
            return false;
        }
        if (lhs == variable_type::function || rhs == variable_type::function) {
            return false;
        }
        return true;
    } else {
        if (lhs == variable_type::function || rhs == variable_type::function) {
            return false;
        }
        return true;
    }
}

class less_node : public binary_node {
 public:
    less_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::less, rhs) {}
 
    value_t *evaluate() const override {
        value_t *left_value = left()->evaluate();
        value_t *right_value = right()->evaluate();

        if (!can_compare(left()->value_type(), token_type::less, right()->value_type())) {
            throw std::runtime_error(
                std::format("cannot compare {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }
        
        if (left_value->type() == variable_type::integer) {
            auto p1 = dynamic_cast<int_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() < p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() < p2->value();
                return new boolean_value(result);
            }
        } else {
            auto p1 = dynamic_cast<float_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() < p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() < p2->value();
                return new boolean_value(result);
            }
        }
    }
};

class greater_node : public binary_node {
 public:
    greater_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::greater, rhs) {}

    value_t *evaluate() const override {
        value_t *left_value = left()->evaluate();
        value_t *right_value = right()->evaluate();

        if (!can_compare(left()->value_type(), token_type::greater, right()->value_type())) {
            throw std::runtime_error(
                std::format("cannot compare {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }
        
        if (left_value->type() == variable_type::integer) {
            auto p1 = dynamic_cast<int_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() > p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() > p2->value();
                return new boolean_value(result);
            }
        } else {
            auto p1 = dynamic_cast<float_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() > p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() > p2->value();
                return new boolean_value(result);
            }
        }
    }
};

class equal_node : public binary_node {
 public:
    equal_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::equal, rhs) {}

    value_t *evaluate() const override {
        value_t *left_value = left()->evaluate();
        value_t *right_value = right()->evaluate();

        if (!can_compare(left()->value_type(), token_type::equal, right()->value_type())) {
            throw std::runtime_error(
                std::format("cannot compare {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }
        
        if (left_value->type() == variable_type::integer) {
            auto p1 = dynamic_cast<int_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() == p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() == p2->value();
                return new boolean_value(result);
            }
        } else {
            auto p1 = dynamic_cast<float_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() == p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() == p2->value();
                return new boolean_value(result);
            }
        }
    }
};

class not_equal_node : public binary_node {
 public:
    not_equal_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::not_equal, rhs) {}

    value_t *evaluate() const override {
        value_t *left_value = left()->evaluate();
        value_t *right_value = right()->evaluate();

        if (!can_compare(left()->value_type(), token_type::not_equal, right()->value_type())) {
            throw std::runtime_error(
                std::format("cannot compare {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }
        
        if (left_value->type() == variable_type::integer) {
            auto p1 = dynamic_cast<int_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() != p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() != p2->value();
                return new boolean_value(result);
            }
        } else {
            auto p1 = dynamic_cast<float_value *>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = dynamic_cast<int_value *>(right_value);
                bool result = p1->value() != p2->value();
                return new boolean_value(result);
            } else {
                auto p2 = dynamic_cast<float_value *>(right_value);
                bool result = p1->value() != p2->value();
                return new boolean_value(result);
            }
        }
    }
};

class logical_and_node : public binary_node {
 public:
    logical_and_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::logical_and, rhs) {}
    
    value_t *evaluate() const override {
        // assert(left()->value_type() == variable_type::boolean);
        // assert(right()->value_type() == variable_type::boolean);

        if (left()->value_type() != variable_type::boolean ||
            right()->value_type() != variable_type::boolean) {
            throw std::runtime_error(
                std::format("invalid && between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        value_t *lhs_value = left()->evaluate();
        value_t *rhs_value = right()->evaluate();

        auto p1 = dynamic_cast<boolean_value *>(lhs_value);
        auto p2 = dynamic_cast<boolean_value *>(rhs_value);

        if (p1->value() == false)
            return new boolean_value(false);

        return new boolean_value(p1->value() && p2->value());
    }
};

class logical_or_node : public binary_node {
 public:
    logical_or_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::logical_or, rhs) {}
    
    value_t *evaluate() const override {
        // assert(left()->value_type() == variable_type::boolean);
        // assert(right()->value_type() == variable_type::boolean);

        if (left()->value_type() != variable_type::boolean ||
            right()->value_type() != variable_type::boolean) {
            throw std::runtime_error(
                std::format("invalid || between {} and {}",
                    variable_type_name(left()->value_type()), variable_type_name(right()->value_type()))
            );
        }

        value_t *lhs_value = left()->evaluate();
        value_t *rhs_value = right()->evaluate();

        auto p1 = dynamic_cast<boolean_value *>(lhs_value);
        auto p2 = dynamic_cast<boolean_value *>(rhs_value);

        if (p1->value() == true)
            return new boolean_value(true);

        return new boolean_value(p1->value() || p2->value());
    }
};

class variable_node : public expression_node {
 public:
    variable_node(std::string_view name, variable_type type)
        : expression_node(ast_node_type::var_node), var_name(name), var_type(type) {
        // if (!program_scope.current_scope().contains(var_name)) {
        //     throw std::runtime_error(
        //         std::format("{} is not defined", var_name)
        //     );
        // }
        // var_ = program_scope.current_scope().find(var_name);
        auto result = static_symbol_table.find(name);
        if (!result.has_value()) {
            throw std::runtime_error(
                std::format("{} is not defined", var_name)
            );
        }
        auto [n, t] = result.value();

        assert(t != variable_type::error);

        if (t == variable_type::integer) {
            set_value_type(variable_type::integer);
        }
        if (t == variable_type::floating) {
            set_value_type(variable_type::floating);
        }
        if (t == variable_type::boolean) {
            set_value_type(variable_type::boolean);
        }
    }

    const std::string &variable_name() const {
        return var_name;
    }

    value_t *evaluate() const override {
        if (var_type == variable_type::error) {
            throw std::runtime_error(
                std::format("{} is not declared", var_name)
            );
        }
        auto var_ = program_scope.current_scope().find(var_name);

        assert(var_ != nullptr);

        if (var_->type() == variable_type::integer) {
            return new int_value(std::dynamic_pointer_cast<variable_int>(var_)->value());
        }
        if (var_->type() == variable_type::floating) {
            return new float_value(std::dynamic_pointer_cast<variable_float>(var_)->value());
        }
        if (var_->type() == variable_type::boolean) {
            return new boolean_value(std::dynamic_pointer_cast<variable_boolean>(var_)->value());
        }
        throw std::runtime_error(
            std::format("invalid assignment type {}", variable_type_name(var_->type()))
        );
    }

 private:
    // std::shared_ptr<variable> var_;
    std::string var_name;
    variable_type var_type;
};

class assign_node : public binary_node {
 public:
    assign_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::assign, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();

        auto l = std::dynamic_pointer_cast<variable_node>(left());
        var_name = l->variable_name();

        if (lhs_type == variable_type::integer) {
            if (rhs_type != variable_type::integer && rhs_type != variable_type::floating) {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}", variable_type_name(rhs_type),
                        variable_type_name(lhs_type))
                );
            }
        } else if (lhs_type == variable_type::floating) {
            if (rhs_type != variable_type::integer && rhs_type != variable_type::floating) {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}", variable_type_name(rhs_type),
                        variable_type_name(lhs_type))
                );
            }
        } else if (lhs_type == variable_type::boolean) {
            if (rhs_type != variable_type::boolean) {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}", variable_type_name(rhs_type),
                        variable_type_name(lhs_type))
                );
            }
        } else if (lhs_type == variable_type::error) {

        } else {
            throw std::runtime_error(
                    std::format("{} cannot be assign",
                        variable_type_name(lhs_type))
                );
        }
    }

    value_t *evaluate() const override {
        if (!program_scope.current_scope().contains(var_name)) {
            throw std::runtime_error(
                std::format("{} is not defined", var_name)
            );
        }
        auto var = program_scope.current_scope().find(var_name);
        if (left()->value_type() == variable_type::integer) {
            auto l = std::dynamic_pointer_cast<variable_int>(var);
            if (right()->value_type() == variable_type::integer) {
                auto p = right()->evaluate();
                auto pv = dynamic_cast<int_value *>(p);
                program_scope.current_scope().set<int32_t>(var_name, pv->value());
                return p;
            } else if (right()->value_type() == variable_type::floating) {
                auto p = right()->evaluate();
                auto pv = dynamic_cast<float_value *>(p);
                program_scope.current_scope().set<int32_t>(var_name, static_cast<int32_t>(pv->value()));
                return p;
            } else {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}",
                        variable_type_name(right()->value_type()), variable_type_name(left()->value_type()))
                );
            }
        } else if (left()->value_type() == variable_type::floating) {
            auto l = std::dynamic_pointer_cast<variable_float>(var);
            if (right()->value_type() == variable_type::integer) {
                auto p = right()->evaluate();
                auto pv = dynamic_cast<int_value *>(p);
                program_scope.current_scope().set<float>(var_name, static_cast<float>(pv->value()));
                return p;
            } else if (right()->value_type() == variable_type::floating) {
                auto p = right()->evaluate();
                auto pv = dynamic_cast<float_value *>(p);
                program_scope.current_scope().set<float>(var_name, pv->value());
                return p;
            } else {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}",
                        variable_type_name(right()->value_type()), variable_type_name(left()->value_type()))
                );
            }
        } else if (left()->value_type() == variable_type::boolean) {
            if (right()->value_type() != variable_type::boolean) {
                throw std::runtime_error(
                    std::format("cannot assign {} to {}",
                        variable_type_name(right()->value_type()), variable_type_name(left()->value_type()))
                );
            }
            auto p = right()->evaluate();
            auto v = dynamic_cast<boolean_value *>(p);
            program_scope.current_scope().set<bool>(var_name, v->value());
            return p;
        } else {
            throw std::runtime_error(
                std::format("{} cannot be assigned",
                    variable_type_name(left()->value_type()))
            );
        }
    }

 private:
    std::string var_name;
};

class negative_node : public unary_node {
 public:
    negative_node(expression_node *value)
        : unary_node(value) {

        assert(value->value_type() != variable_type::error);

        set_value_type(value->value_type());
    }

    value_t *evaluate() const override {
        value_t *val = value()->evaluate();
        if (value_type() == variable_type::integer) {
            auto v = dynamic_cast<int_value *>(val);
            auto result = new int_value(-v->value());
            delete val;
            return result;
        } else if (value_type() == variable_type::floating) {
            auto v = dynamic_cast<float_value *>(val);
            auto result = new float_value(-v->value());
            delete val;
            return result;
        } else {
            return new error_value(
                std::format("invalid operator - on {}", variable_type_name(value_type()))
            );
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

class expr_statement_node : public statement_node, expression_node {
 public:
    expr_statement_node(expression_node *expr)
        : statement_node(ast_node_type::expr_statement),
        expression_node(ast_node_type::expr_statement), expr_(expr) {}
    
    std::pair<execute_state, value_t *> execute() override {
        expr_->evaluate();
        return {execute_state::normal, nullptr};
    }

    value_t *evaluate() const override {
        return expr_->evaluate();
    }

    variable_type value_type() const {
        return expr_->value_type();
    }

 private:
    std::shared_ptr<expression_node> expr_;

};

class declaration_node : public statement_node {
 public:
    declaration_node(variable_type type, std::string_view name, expression_node *value)
        : statement_node(ast_node_type::declaration), type_(type), variable_name_(name), init_value_(value) {
        assert(type != variable_type::error);
        std::println("declaration");
        // assert(value->value_type() != variable_type::error);
        if (value != nullptr && value->value_type() == variable_type::boolean && type == variable_type::boolean) {
            
        }
        else if (value != nullptr && variable_type_cast(type, value->value_type()) == variable_type::error) {
            throw std::runtime_error(
                std::format("initial value type '{}' is not the same with variable type '{}",
                    variable_type_name(value->value_type()), variable_type_name(type))
            );
        }
        
        switch (type) {
            case variable_type::integer:
            case variable_type::floating:
            case variable_type::boolean:
                break;
            default:
                throw std::runtime_error("invalid variable type");
        }

        static_symbol_table.insert(name, type);
        
        if (value == nullptr) {
            switch (type) {
                case variable_type::integer:
                    init_value_ = std::make_shared<int_node>(0);
                    break;
                case variable_type::floating:
                    init_value_ = std::make_shared<float_node>(0.0f);
                    break;
                case variable_type::boolean:
                    init_value_ = std::make_shared<boolean_node>(false);
                    break;
                default:
                    throw std::runtime_error("unreachable code");
            }
            // init_value_.reset(value);
            assert(init_value_ != nullptr);
        }
    }

    variable_type type() const {
        return type_;
    }

    void set_init_value(expression_node *expr) {
        assert(type_ != variable_type::error);
        assert(expr->value_type() != variable_type::error);
        assert(variable_type_cast(type_, expr->value_type()) != variable_type::error);

        init_value_.reset(expr);
        type_ = expr->value_type();
    }

    std::pair<execute_state, value_t *> execute() override {
        // program_scope.current_scope().insert()
        std::println("declaration execute");
        std::println("type: {}", variable_type_name(type_));
        std::println("init type: {}", variable_type_name(init_value_->value_type()));
        switch (type_) {
            case variable_type::integer: {
                // auto p = std::dynamic_pointer_cast<int_node>(init_value_);
                auto p = init_value_;
                // assert(p != nullptr);
                // assert(init_value_->node_type() == ast_node_type::func_call);
                value_t *r = p->evaluate();
                assert(r != nullptr);
                auto v = dynamic_cast<int_value *>(r);
                assert(v != nullptr);
                program_scope.current_scope().insert(new variable_int(variable_name_, v->value()));
            }
            break;
            case variable_type::floating: {
                auto p = std::dynamic_pointer_cast<float_node>(init_value_);
                value_t *r = p->evaluate();
                auto v = dynamic_cast<float_value *>(r);
                program_scope.current_scope().insert(new variable_float(variable_name_, v->value()));
            }
            break;
            case variable_type::boolean: {
                auto p = std::dynamic_pointer_cast<boolean_node>(init_value_);
                value_t *r = p->evaluate();
                auto v = dynamic_cast<boolean_value *>(r);
                program_scope.current_scope().insert(new variable_boolean(variable_name_, v->value()));
            }
            break;
            default:
                throw std::runtime_error("invalid variable type (unreachable code)");
        }
        std::println("declaration end");
        return {execute_state::normal, nullptr};
    }

    variable_type value_type() const {
        return type_;
    }

    const std::string &name() const {
        return variable_name_;
    }

    std::shared_ptr<expression_node> init_value() const {
        return init_value_;
    }

 private:
    variable_type type_;
    std::string variable_name_;
    std::shared_ptr<expression_node> init_value_;
};


class block_node : public statement_node {
 public:
    block_node() : statement_node(ast_node_type::block) {

    }

    void insert(statement_node *statement) {
        statements_.emplace_back(statement);
    }

    const std::vector<std::shared_ptr<statement_node>> &statements() const {
        return statements_;
    }

    std::pair<execute_state, value_t *> execute() override {
        std::println("block node execute");
        std::println("size: {}", statements_.size());
        // assert(statements_.size() == 1);
        for (auto &statement : statements_) {
            std::println("loop");
            std::println("not type {}", static_cast<int>(statement->node_type()));
            // assert(statement->node_type() == ast_node_type::jump);
            std::pair<execute_state, value_t *> result = statement->execute();
            auto [state, value] = result;
            if (state != execute_state::normal) {
                return result;
            }
        }
        return {execute_state::normal, nullptr};
    }

 private:
    std::vector<std::shared_ptr<statement_node>> statements_;
};

class void_node : public expression_node {
 public:
    void_node() : expression_node(ast_node_type::empty) {}

    value_t *evaluate() const override {
        return new int_value(0);
    }

};

class for_node : public statement_node {
 public:
    for_node(expr_statement_node *init, expr_statement_node *condition,
        expression_node *update, statement_node *block)
        : statement_node(ast_node_type::node_for), init_statement_(init),
          condition_(condition), update_(update), statements_(block) {}

    std::pair<execute_state, value_t *> execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        init_statement_->execute();
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
            std::pair<execute_state, value_t *> result = statements_->execute();
            auto [state, value] = result;
            if (state == execute_state::continued) {
                update_->evaluate();
                continue;
            } else if (state == execute_state::broken) {
                break;
            } else if (state == execute_state::returned) {
                return {execute_state::returned, value};
            }
            update_->evaluate();
        }
        return {execute_state::normal, nullptr};
    }

 private:
    std::shared_ptr<expr_statement_node> init_statement_;
    std::shared_ptr<expr_statement_node> condition_;
    std::shared_ptr<expression_node> update_;
    std::shared_ptr<statement_node> statements_;
};

class while_node : public statement_node {
 public:
    while_node(expression_node *condition, statement_node *body)
        : statement_node(ast_node_type::node_while),
          condition_(condition), statements_(body) {}

    std::pair<execute_state, value_t *> execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
            std::pair<execute_state, value_t *> result = statements_->execute();
            auto [state, value] = result;
            if (state == execute_state::continued) {
                continue;
            } else if (state == execute_state::broken) {
                break;
            } else if (state == execute_state::returned) {
                return result;
            }
        }
        return {execute_state::normal, nullptr};
    }
 private:
    std::shared_ptr<expression_node> condition_;
    std::shared_ptr<statement_node> statements_;
};

class func_decl_node : public statement_node {
 public:
    func_decl_node(variable_type type, std::string_view name, statement_node *body)
        : statement_node(ast_node_type::func_decl),
          return_type_(type), name_(name), body_(body) {}

    std::pair<execute_state, value_t *> execute() override {
        // std::pair<execute_state, value_t *> result = body_->execute();
        // auto [state, value] = result;
        // if (state == execute_state::returned) {
        //     return {execute_state::normal, value};
        // } else if (state == execute_state::continued) {
        //     throw std::runtime_error("continue must in a loop");
        // } else {
        //     throw std::runtime_error("break must in a loop");
        // }
        
        func_decls.add(name_, this);
        std::println("func_decl execute");
        return {execute_state::normal, nullptr};
        
    }

    void add_param(declaration_node *dec) {
        param_.emplace_back(dec);
    }

    variable_type return_type() const {
        return return_type_;
    }

    const std::vector<std::shared_ptr<declaration_node>> &params() const {
        return param_;
    }

    const std::string &name() const {
        return name_;
    }

    std::shared_ptr<statement_node> body() {
        return body_;
    }

 private:
    variable_type return_type_;
    std::string name_;
    std::vector<std::shared_ptr<declaration_node>> param_;
    std::shared_ptr<statement_node> body_;
};

class continue_node : public statement_node {
 public:
    continue_node() : statement_node(ast_node_type::jump) {}

    std::pair<execute_state, value_t *> execute() override {
        return {execute_state::continued, nullptr};
    }
};

class break_node : public statement_node {
 public:
    break_node() : statement_node(ast_node_type::jump) {}

    std::pair<execute_state, value_t *> execute() override {
        return {execute_state::broken, nullptr};
    }
};

class return_node : public statement_node {
 public:
    return_node(expression_node *expr)
        : statement_node(ast_node_type::jump), expr_(expr) {}

    std::pair<execute_state, value_t *> execute() override {
        if (expr_ == nullptr)
            return {execute_state::returned, nullptr};
        std::println("return execute");
        assert(expr_->node_type() == ast_node_type::binary);
        return {execute_state::returned, expr_->evaluate()};
    }

 private:
    std::shared_ptr<expression_node> expr_;
};

class func_call_node : public expression_node {
 public:
    func_call_node(std::string_view name, std::vector<expression_node *> args)
        : expression_node(ast_node_type::func_call), name_(name) {
        for (auto arg : args) {
            args_.emplace_back(arg);
        }
    }
    
    value_t *evaluate() const override {
        auto func = func_decls.find(name_);
        assert(func != nullptr);
        variable_type type = func->return_type();
        // return func->execute().second;
        assert(func->body() != nullptr);
        std::println("func_call evaluate");

        for (auto &p : func->params()) {
            // program_scope.current_scope().insert();
            if (p->value_type() == variable_type::integer) {
                auto init = p->init_value();
                value_t *v = init->evaluate();
                auto i = dynamic_cast<int_value *>(v);
                program_scope.current_scope().insert(new variable_int(p->name(), i->value()));
            } else if (p->value_type() == variable_type::floating) {
                auto init = p->init_value();
                value_t *v = init->evaluate();
                auto i = dynamic_cast<float_value *>(v);
                program_scope.current_scope().insert(new variable_float(p->name(), i->value()));
            } else if (p->value_type() == variable_type::boolean) {
                auto init = p->init_value();
                value_t *v = init->evaluate();
                auto i = dynamic_cast<boolean_value *>(v);
                program_scope.current_scope().insert(new variable_boolean(p->name(), i->value()));
            } else {
                throw std::runtime_error("invalid function argument");
            }
        }
        
        assert(func->body()->node_type() == ast_node_type::block);
        auto ttt = std::dynamic_pointer_cast<block_node>(func->body());
        assert(ttt != nullptr);
        assert(ttt->statements()[0]->node_type() == ast_node_type::jump);

        auto [state, value] = func->body()->execute();
        std::println("func_call evaluate end");
        return value;
    }

    variable_type value_type() const override {
        auto func = static_func_decls.find(name_);
        return func->return_type();
    }

 private:
    std::string name_;
    std::vector<std::shared_ptr<expression_node>> args_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif