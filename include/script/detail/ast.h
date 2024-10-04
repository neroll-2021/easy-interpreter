#ifndef NEROLL_SCRIPT_DETAIL_AST_H
#define NEROLL_SCRIPT_DETAIL_AST_H

#include <cstdint>  // int32_t
#include <memory>   // std::shared_ptr
#include <vector>   // std::vector
#include <format>   // std::format
#include <iostream>

#include "script/detail/variable.h"     // arithmetic_type_cast
#include "script/detail/value_t.h"      // value_t
#include "script/detail/operator.h"     // plus, minus, multiplies, divides, modulus
#include "script/detail/scope.h"        // program_scope
#include "script/detail/static_symbols.h"
#include "script/detail/function.h"
#include "script/detail/exception.h"

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

    virtual std::shared_ptr<value_t> evaluate() const = 0;

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

    virtual std::pair<execute_state, std::shared_ptr<value_t>> execute() = 0;
};

bool is_both_boolean(variable_type lhs, variable_type rhs) {
    return lhs == variable_type::boolean && rhs == variable_type::boolean;
}

bool is_both_integer(variable_type lhs, variable_type rhs) {
    return lhs == variable_type::integer && rhs == variable_type::integer;
}

bool is_both_integer(const std::shared_ptr<expression_node> &lhs, const std::shared_ptr<expression_node> &rhs) {
    return is_both_integer(lhs->value_type(), rhs->value_type());
}

bool is_arithmetic_operator(token_type type) {
    return type == token_type::plus || type == token_type::minus ||
           type == token_type::asterisk || type == token_type::slash;
}

bool is_relation_oeprator(token_type type) {
    return type == token_type::less || type == token_type::greater ||
           type == token_type::equal || type == token_type::not_equal;
}

bool is_logical_operator(token_type type) {
    return type == token_type::logical_and || type == token_type::logical_or;
}

bool is_modulus_operator(token_type type) {
    return type == token_type::mod;
}

bool is_assign_operator(token_type type) {
    return type == token_type::assign;
}

bool is_equality_operator(token_type op) {
    return op == token_type::equal || op == token_type::not_equal;
}

// TODO
// 
variable_type binary_expression_type(variable_type lhs_type, token_type op, variable_type rhs_type) {
    assert(lhs_type != variable_type::error);
    assert(rhs_type != variable_type::error);
    
    if (is_arithmetic_operator(op)) {
        if (arithmetic_type_cast(lhs_type, rhs_type) == variable_type::error)
            throw_type_error("invalid operator {} betweem {} and {}", op, lhs_type, rhs_type);
        return arithmetic_type_cast(lhs_type, rhs_type);
    } else if (is_modulus_operator(op)) {
        if (is_both_integer(lhs_type, rhs_type))
            return variable_type::integer;
        throw_type_error("invalid operator % between {} and {}", lhs_type, rhs_type);
    } else if (is_relation_oeprator(op)) {
        if (is_both_boolean(lhs_type, rhs_type) && !is_equality_operator(op))
            throw_type_error("invalid operator {} between {} and {}", op, lhs_type, rhs_type);
        
        return variable_type::boolean;
    } else if (is_logical_operator(op)) {
        if (is_both_boolean(lhs_type, rhs_type))
            return variable_type::boolean;
        throw_type_error("invalid operator {} between {} and {}", lhs_type, rhs_type);
    } else if (is_assign_operator(op)) {
        return lhs_type;
    } else {
        throw_syntax_error("invalid operator '{}'", op);
    }
}

bool is_valid_binary_expr(variable_type lhs, token_type op, variable_type rhs) {
    if (is_arithmetic_operator(op))
        return arithmetic_type_cast(lhs, rhs) != variable_type::error;

    if (is_modulus_operator(op))
        return is_both_integer(lhs, rhs);
    
    if (is_logical_operator(op))
        return is_both_boolean(lhs, rhs);
    
    if (is_relation_oeprator(op)) {
        // if (!is_both_boolean(lhs, rhs))
        return true;
    }
    return false;
}

class binary_node : public expression_node {
 public:
    binary_node(expression_node *lhs, token_type op, expression_node *rhs)
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

class binary_arithmetic_node : public binary_node {
 public:
    binary_arithmetic_node(expression_node *lhs, token_type op, expression_node *rhs)
        : binary_node(lhs, op, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();
        variable_type type = arithmetic_type_cast(lhs_type, rhs_type);
        if (type == variable_type::error)
            throw_type_error("invalid operator {} between {} and {}", op, lhs_type, rhs_type);
        set_value_type(type);
    }

    std::shared_ptr<value_t> select_operator(token_type op) const;

    template <typename Op>
    std::shared_ptr<value_t> evaluate_result() const {
        assert(value_type() != variable_type::error);
        assert(left() != nullptr);
        assert(right() != nullptr);

        // @bug 
        std::shared_ptr<value_t> lhs_value = left()->evaluate();
        std::shared_ptr<value_t> rhs_value = right()->evaluate();

        if (is_both_integer(left(), right())) {
            auto l = std::dynamic_pointer_cast<int_value>(lhs_value);
            auto r = std::dynamic_pointer_cast<int_value>(rhs_value);
            return std::make_shared<int_value>(Op{}(l->value(), r->value()));
        } else if (left()->value_type() == variable_type::integer) {
            auto l = std::dynamic_pointer_cast<int_value>(lhs_value);
            auto r = std::dynamic_pointer_cast<float_value>(rhs_value);
            return std::make_shared<float_value>(Op{}(l->value(), r->value()));
        } else if (right()->value_type() == variable_type::integer) {
            auto l = std::dynamic_pointer_cast<float_value>(lhs_value);
            auto r = std::dynamic_pointer_cast<int_value>(rhs_value);
            return std::make_shared<float_value>(Op{}(l->value(), r->value()));
        } else {
            auto l = std::dynamic_pointer_cast<float_value>(lhs_value);
            auto r = std::dynamic_pointer_cast<float_value>(rhs_value);
            return std::make_shared<float_value>(Op{}(l->value(), r->value()));
        }
    }
};

template <>
std::shared_ptr<value_t> binary_arithmetic_node::evaluate_result<modulus>() const {
    assert(left()->value_type() == variable_type::integer);
    assert(right()->value_type() == variable_type::integer);

    std::shared_ptr<value_t> lhs_value = left()->evaluate();
    std::shared_ptr<value_t> rhs_value = right()->evaluate();

    auto lhs = std::dynamic_pointer_cast<int_value>(lhs_value);
    auto rhs = std::dynamic_pointer_cast<int_value>(rhs_value);

    return std::make_shared<int_value>(lhs->value() % rhs->value());
}

std::shared_ptr<value_t> binary_arithmetic_node::select_operator(token_type op) const {
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
            throw_syntax_error("invalid operator {}", op);
    }
}

class add_node final : public binary_arithmetic_node {
 public:
    add_node(expression_node *lhs, expression_node *rhs)
        : binary_arithmetic_node(lhs, token_type::plus, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::plus);
    }
};

class minus_node final : public binary_arithmetic_node {
 public:
    minus_node(expression_node *lhs, expression_node *rhs)
        : binary_arithmetic_node(lhs, token_type::minus, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::minus);
    }
};

class multiply_node final : public binary_arithmetic_node {
 public:
    multiply_node(expression_node *lhs, expression_node *rhs)
        : binary_arithmetic_node(lhs, token_type::asterisk, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::asterisk);
    }
};

class divide_node final : public binary_arithmetic_node {
 public:
    divide_node(expression_node *lhs, expression_node *rhs)
        : binary_arithmetic_node(lhs, token_type::slash, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();

        if (!is_both_integer(lhs_type, rhs_type)) {
            throw_type_error("invalid operator % between {} and {}", lhs_type, rhs_type);
        }
    }

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::slash);
    }
};

class modulus_node final : public binary_arithmetic_node {
 public:
    modulus_node(expression_node *lhs, expression_node *rhs)
        : binary_arithmetic_node(lhs, token_type::mod, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();

        if (!is_both_integer(lhs_type, rhs_type))
            throw_type_error(
                "invalid operator % between {} and {}", lhs_type, rhs_type
            );
        set_value_type(variable_type::integer);
    }

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::mod);
    }
};

bool can_compare(variable_type lhs, variable_type rhs) {
    constexpr static std::array<std::array<int, 4>, 4> table{{{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};
    auto lhs_index = static_cast<std::size_t>(lhs);
    auto rhs_index = static_cast<std::size_t>(rhs);
    return static_cast<bool>(table[lhs_index][rhs_index]);
}

bool can_equal(variable_type lhs, variable_type rhs) {
    constexpr static std::array<std::array<int, 4>, 4> table{{{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}}};
    auto lhs_index = static_cast<std::size_t>(lhs);
    auto rhs_index = static_cast<std::size_t>(rhs);
    return static_cast<bool>(table[lhs_index][rhs_index]);
}

class relation_node : public binary_node {
 public:
    relation_node(expression_node *lhs, token_type op, expression_node *rhs)
        : binary_node(lhs, op, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();

        if (is_equality_operator(op)) {
            if (!can_equal(lhs_type, rhs_type))
                throw_type_error(
                    "invalid operator {} between {} and {}", op, lhs_type, rhs_type
                );
        } else if (is_relation_oeprator(op)) {
            if (!can_compare(lhs_type, rhs_type))
                throw_type_error(
                    "invaild operator {} between {} and {}", op, lhs_type, rhs_type
                );
        }

        set_value_type(variable_type::boolean);
    }

    template <typename Op>
    std::shared_ptr<value_t> evaluate_result() const {
        std::shared_ptr<value_t> left_value = left()->evaluate();
        std::shared_ptr<value_t> right_value = right()->evaluate();

        if (left_value->type() == variable_type::integer) {
            auto p1 = std::dynamic_pointer_cast<int_value>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = std::dynamic_pointer_cast<int_value>(right_value);
                bool result = Op{}(p1->value(), p2->value());
                return std::make_shared<boolean_value>(result);
            } else {
                auto p2 = std::dynamic_pointer_cast<float_value>(right_value);
                bool result = Op{}(p1->value(), p2->value());
                return std::make_shared<boolean_value>(result);
            }
        } else {
            auto p1 = std::dynamic_pointer_cast<float_value>(left_value);
            if (right_value->type() == variable_type::integer) {
                auto p2 = std::dynamic_pointer_cast<int_value>(right_value);
                bool result = Op{}(p1->value(), p2->value());
                return std::make_shared<boolean_value>(result);
            } else {
                auto p2 = std::dynamic_pointer_cast<float_value>(right_value);
                bool result = Op{}(p1->value(), p2->value());
                return std::make_shared<boolean_value>(result);
            }
        }
    }

    std::shared_ptr<value_t> select_operator(token_type op) const {
        switch (op) {
            case token_type::less:
                return evaluate_result<less>();
            case token_type::greater:
                return evaluate_result<greater>();
            case token_type::equal:
                return evaluate_result<equal>();
            case token_type::not_equal:
                return evaluate_result<not_equal>();
            default:
                throw_syntax_error("invalid relation operator {}", op);
        }
    }
};

class less_node : public relation_node {
 public:
    less_node(expression_node *lhs, expression_node *rhs)
        : relation_node(lhs, token_type::less, rhs) {}
 
    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::less);
    }
};

class greater_node : public relation_node {
 public:
    greater_node(expression_node *lhs, expression_node *rhs)
        : relation_node(lhs, token_type::greater, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::greater);
    }
};

class equal_node : public relation_node {
 public:
    equal_node(expression_node *lhs, expression_node *rhs)
        : relation_node(lhs, token_type::equal, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::equal);
    }
};

class not_equal_node : public relation_node {
 public:
    not_equal_node(expression_node *lhs, expression_node *rhs)
        : relation_node(lhs, token_type::not_equal, rhs) {}

    std::shared_ptr<value_t> evaluate() const override {
        return select_operator(token_type::not_equal);
    }
};

class logical_and_node : public binary_node {
 public:
    logical_and_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::logical_and, rhs) {
        variable_type lhs_type = left()->value_type();
        variable_type rhs_type = right()->value_type();
        if (!is_both_boolean(lhs_type, rhs_type)) {
            throw_type_error("invalid '&&' between {} and {}", lhs_type, rhs_type);
        }
    }
    
    std::shared_ptr<value_t> evaluate() const override {

        std::shared_ptr<value_t> lhs_value = left()->evaluate();
        

        auto p1 = std::dynamic_pointer_cast<boolean_value>(lhs_value);
        

        if (p1->value() == false)
            return std::make_shared<boolean_value>(false);
        

        std::shared_ptr<value_t> rhs_value = right()->evaluate();
        auto p2 = std::dynamic_pointer_cast<boolean_value>(rhs_value);

        return std::make_shared<boolean_value>(p1->value() && p2->value());
    }
};

class logical_or_node : public binary_node {
 public:
    logical_or_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::logical_or, rhs) {
        variable_type lhs_type = left()->value_type();
        variable_type rhs_type = right()->value_type();
        if (!is_both_boolean(lhs_type, rhs_type)) {
            throw_type_error("invalid '||' between {} and {}", lhs_type, rhs_type);
        }
    }
    
    std::shared_ptr<value_t> evaluate() const override {

        std::shared_ptr<value_t> lhs_value = left()->evaluate();

        auto p1 = std::dynamic_pointer_cast<boolean_value>(lhs_value);

        if (p1->value() == true)
            return std::make_shared<boolean_value>(true);
        
        std::shared_ptr<value_t> rhs_value = right()->evaluate();
        auto p2 = std::dynamic_pointer_cast<boolean_value>(rhs_value);

        return std::make_shared<boolean_value>(p1->value() || p2->value());
    }
};

std::shared_ptr<value_t> variable_value(const std::shared_ptr<variable> &var) {
    switch (var->type()) {
        case variable_type::integer:
            return std::make_shared<int_value>(std::dynamic_pointer_cast<variable_int>(var)->value());
        case variable_type::floating:
            return std::make_shared<float_value>(std::dynamic_pointer_cast<variable_float>(var)->value());
        case variable_type::boolean:
            return std::make_shared<boolean_value>(std::dynamic_pointer_cast<variable_boolean>(var)->value());
        default:
            return nullptr;
    }
}

class variable_node : public expression_node {
 public:
    variable_node(std::string_view name, variable_type type)
        : expression_node(ast_node_type::var_node), var_name(name), var_type(type) {
        auto result = static_symbol_table.find(name);
        if (!result.has_value()) {
            throw_symbol_error("{} is not defined", name);
        }
        auto [n, t] = result.value();

        assert(t != variable_type::error);
        assert(t != variable_type::function);
        
        set_value_type(t);
    }

    const std::string &variable_name() const {
        return var_name;
    }

    std::shared_ptr<value_t> evaluate() const override {
        assert(var_type != variable_type::error);
        if (var_type == variable_type::error) {
            throw_symbol_error("{} is not defined", var_name);
        }
        auto var_ = program_scope.find(var_name);

        assert(var_ != nullptr);

        std::shared_ptr<value_t> p = variable_value(var_);
        if (p == nullptr) {
            throw_type_error(
                "invalid variable type {}", var_->type()
            );
        }
        return p;
    }

 private:
    std::string var_name;
    variable_type var_type;
};

bool can_assign(variable_type lhs, variable_type rhs) {
    assert(lhs != variable_type::error);
    assert(rhs != variable_type::error);
    constexpr static std::array<std::array<int, 4>, 4> table = {{{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}}};
    auto lhs_index = static_cast<std::size_t>(lhs);
    auto rhs_index = static_cast<std::size_t>(rhs);
    return table[lhs_index][rhs_index];
}

class assign_node : public binary_node {
 public:
    assign_node(expression_node *lhs, expression_node *rhs)
        : binary_node(lhs, token_type::assign, rhs) {
        variable_type lhs_type = lhs->value_type();
        variable_type rhs_type = rhs->value_type();

        auto l = std::dynamic_pointer_cast<variable_node>(left());
        assert(l != nullptr);
        var_name = l->variable_name();

        if (!can_assign(lhs_type, rhs_type)) {
            throw_type_error("cannot assign {} to {}", rhs_type, lhs_type);
        }
    }

    std::shared_ptr<value_t> evaluate() const override {
        if (!program_scope.current_scope().contains(var_name)) {
            throw_symbol_error("{} is not defined", var_name);
        }
        auto var = program_scope.current_scope().find(var_name);
        if (left()->value_type() == variable_type::integer) {
            auto l = std::dynamic_pointer_cast<variable_int>(var);
            if (right()->value_type() == variable_type::integer) {
                auto p = right()->evaluate();
                auto pv = std::dynamic_pointer_cast<int_value>(p);
                program_scope.current_scope().set<int32_t>(var_name, pv->value());
                return p;
            } else if (right()->value_type() == variable_type::floating) {
                auto p = right()->evaluate();
                auto pv = std::dynamic_pointer_cast<float_value>(p);
                program_scope.current_scope().set<int32_t>(var_name, static_cast<int32_t>(pv->value()));
                return p;
            } else {
                throw_type_error(
                    "cannot assign {} to {}", right()->value_type(), left()->value_type()
                );
            }
        } else if (left()->value_type() == variable_type::floating) {
            auto l = std::dynamic_pointer_cast<variable_float>(var);
            if (right()->value_type() == variable_type::integer) {
                auto p = right()->evaluate();
                auto pv = std::dynamic_pointer_cast<int_value>(p);
                program_scope.current_scope().set<float>(var_name, static_cast<float>(pv->value()));
                return p;
            } else if (right()->value_type() == variable_type::floating) {
                auto p = right()->evaluate();
                auto pv = std::dynamic_pointer_cast<float_value>(p);
                program_scope.current_scope().set<float>(var_name, pv->value());
                return p;
            } else {
                throw_type_error(
                    "cannot assign {} to {}", right()->value_type(), left()->value_type()
                );
            }
        } else if (left()->value_type() == variable_type::boolean) {
            if (right()->value_type() != variable_type::boolean) {
                throw_type_error(
                    "cannot assign {} to {}", right()->value_type(), left()->value_type()
                );
            }
            auto p = right()->evaluate();
            auto v = std::dynamic_pointer_cast<boolean_value>(p);
            program_scope.current_scope().set<bool>(var_name, v->value());
            return p;
        } else {
            throw_type_error("{} cannot be assign", left()->value_type());
        }
    }

 private:
    std::string var_name;
};

class negative_node : public unary_node {
 public:
    negative_node(expression_node *value)
        : unary_node(value) {
        variable_type type = value->value_type();
        if (type != variable_type::integer && type != variable_type::floating) {
            throw_type_error("invalid operand type {} for '-'", type);
        }

        set_value_type(value->value_type());
    }

    std::shared_ptr<value_t> evaluate() const override {
        std::shared_ptr<value_t> val = value()->evaluate();
        if (value_type() == variable_type::integer) {
            auto v = std::dynamic_pointer_cast<int_value>(val);
            return std::make_shared<int_value>(-v->value());
        } else if (value_type() == variable_type::floating) {
            auto v = std::dynamic_pointer_cast<float_value>(val);
            return std::make_shared<float_value>(-v->value());
        } else {
            throw_type_error("invalid operator '-' on {}", value_type());
        }
    }
};

class int_node : public expression_node {
 public:
    int_node(int32_t value)
        : expression_node(ast_node_type::integer), value_(value) {
        set_value_type(variable_type::integer);
    }

    std::shared_ptr<value_t> evaluate() const override {
        return std::make_shared<int_value>(value_);
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

    std::shared_ptr<value_t> evaluate() const override {
        return std::make_shared<float_value>(value_);
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

    std::shared_ptr<value_t> evaluate() const override {
        return std::make_shared<boolean_value>(value_);
    }

 private:
    bool value_;
};

class expr_statement_node : public statement_node, expression_node {
 public:
    expr_statement_node(expression_node *expr)
        : statement_node(ast_node_type::expr_statement),
        expression_node(ast_node_type::expr_statement), expr_(expr) {}
    
    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        expr_->evaluate();
        return {execute_state::normal, nullptr};
    }

    std::shared_ptr<value_t> evaluate() const override {
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

        if (static_symbol_table.current_scope().contains(name)) {
            throw_symbol_error(
                "{} is already defined in this scope", name
            );
        }

        if (value != nullptr && !can_assign(type, value->value_type())) {
            throw_type_error("initial value type {} cannot assign to {}", value->value_type(), type);
        }

        
        switch (type) {
            case variable_type::integer:
            case variable_type::floating:
            case variable_type::boolean:
                break;
            default:
                throw_type_error("invalid variable type: {}", type);
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
            assert(init_value_ != nullptr);
        }
    }

    variable_type type() const {
        return type_;
    }

    void set_init_value(std::shared_ptr<expression_node> expr) {
        assert(type_ != variable_type::error);
        assert(expr->value_type() != variable_type::error);
        assert(arithmetic_type_cast(type_, expr->value_type()) != variable_type::error);

        init_value_ = expr;
        type_ = expr->value_type();
    }

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        switch (type_) {
            case variable_type::integer: {
                auto p = init_value_;
                if (p->value_type() == variable_type::floating) {
                    std::shared_ptr<value_t> r = p->evaluate();
                    auto v = std::dynamic_pointer_cast<float_value>(r);
                    program_scope.current_scope().insert(new variable_int(variable_name_, static_cast<int32_t>(v->value())));
                } else {    // int
                    std::shared_ptr<value_t> r = p->evaluate();
                    auto v = std::dynamic_pointer_cast<int_value>(r);
                    program_scope.current_scope().insert(new variable_int(variable_name_, v->value()));
                }
            }
            break;
            case variable_type::floating: {
                auto p = init_value_;
                if (p->value_type() == variable_type::floating) {
                    std::shared_ptr<value_t> r = p->evaluate();
                    auto v = std::dynamic_pointer_cast<float_value>(r);
                    program_scope.current_scope().insert(new variable_float(variable_name_, v->value()));
                } else {    // int
                    std::shared_ptr<value_t> r = p->evaluate();
                    auto v = std::dynamic_pointer_cast<float_value>(r);
                    program_scope.current_scope().insert(new variable_float(variable_name_, static_cast<float>(v->value())));
                }
            }
            break;
            case variable_type::boolean: {
                auto p = std::dynamic_pointer_cast<boolean_node>(init_value_);
                std::shared_ptr<value_t> r = p->evaluate();
                auto v = std::dynamic_pointer_cast<boolean_value>(r);
                program_scope.current_scope().insert(new variable_boolean(variable_name_, v->value()));
            }
            break;
            default:
                throw std::runtime_error("invalid variable type (unreachable code)");
        }
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

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        for (auto &statement : statements_) {
            std::pair<execute_state, std::shared_ptr<value_t>> result = statement->execute();
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

    std::shared_ptr<value_t> evaluate() const override {
        return std::make_shared<int_value>(0);
    }

};

class for_node : public statement_node {
 public:
    for_node(expr_statement_node *init, expr_statement_node *condition,
        expression_node *update, statement_node *block)
        : statement_node(ast_node_type::node_for), init_statement_(init),
          condition_(condition), update_(update), statements_(block) {}

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        init_statement_->execute();
        while (std::dynamic_pointer_cast<boolean_value>(condition_->evaluate())->value()) {
            std::pair<execute_state, std::shared_ptr<value_t>> result = statements_->execute();
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

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        while (std::dynamic_pointer_cast<boolean_value>(condition_->evaluate())->value()) {
            std::pair<execute_state, std::shared_ptr<value_t>> result = statements_->execute();
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

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        func_decls.add(name_, this);
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

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        return {execute_state::continued, nullptr};
    }
};

class break_node : public statement_node {
 public:
    break_node() : statement_node(ast_node_type::jump) {}

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        return {execute_state::broken, nullptr};
    }
};

class return_node : public statement_node {
 public:
    return_node(expression_node *expr)
        : statement_node(ast_node_type::jump), expr_(expr) {}

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        if (expr_ == nullptr)
            return {execute_state::returned, nullptr};
        return {execute_state::returned, expr_->evaluate()};
    }

 private:
    std::shared_ptr<expression_node> expr_;
};

class func_call_node : public expression_node {
 public:
    func_call_node(std::string_view name, const std::vector<expression_node *> &args)
        : expression_node(ast_node_type::func_call), name_(name) {
        for (auto arg : args) {
            args_.emplace_back(arg);
        }
        if (name_ == "input") {
            variable_type arg_type = args_[0]->value_type();
            set_value_type(arg_type);
        } else if (name_ == "println") {
            set_value_type(variable_type::integer);
        } else {
            const func_decl_node *func = static_func_decls.find(name_);
            if (func == nullptr) {
                throw_symbol_error("function {} is not defined", name_);
            }
            set_value_type(func->return_type());
        }
    }

    const std::string &name() const {
        return name_;
    }
    
    std::shared_ptr<value_t> evaluate() const override {

        if (name_ == "input") {
            variable_type arg_type = args_[0]->value_type();
            if (arg_type == variable_type::integer) {
                int32_t value = 0;
                std::cin >> value;
                if (std::cin.bad())
                    std::cin.clear();
                eatline();
                return std::make_shared<int_value>(value);
            } else if (arg_type == variable_type::floating) {
                float value = 0.0;
                std::cin >> value;
                eatline();
                return std::make_shared<float_value>(value);
            } else if (arg_type == variable_type::boolean) {
                std::string s, t;
                bool value = false;
                std::getline(std::cin, s);
                if (s == "true")
                    return std::make_shared<boolean_value>(true);
                else if (s == "false")
                    return std::make_shared<boolean_value>(false);
                else
                    throw_type_error(
                        "must input 'true' or 'false'"
                    );
            } else {
                throw_type_error(
                    "invalid aargument type {} for input",
                    arg_type
                );
            }
        }

        if (name_ == "println") {
            variable_type arg_type = args_[0]->value_type();
            if (arg_type == variable_type::integer) {
                auto p = std::dynamic_pointer_cast<int_value>(args_[0]->evaluate());
                std::println("{}", p->value());
            } else if (arg_type == variable_type::floating) {
                auto p = std::dynamic_pointer_cast<float_value>(args_[0]->evaluate());
                std::println("{}", p->value());
            } else if (arg_type == variable_type::boolean) {
                auto p = std::dynamic_pointer_cast<boolean_value>(args_[0]->evaluate());
                std::println("{}", p->value());
            } else {
                throw_type_error(
                    "invalid aargument type {} for println",
                    arg_type
                );
            }
            return std::make_shared<int_value>(0);
        }

        func_decl_node *func = func_decls.find(name_);
        assert(func != nullptr);
        assert(args_.size() == func->params().size());

        program_scope.push_scope();

        variable_type type = func->return_type();

        assert(func->body() != nullptr);

        const auto &params = func->params();
        for (std::size_t i = 0; i < args_.size(); i++) {
            if (params[i]->value_type() == variable_type::integer) {
                if (args_[i]->value_type() == variable_type::integer) {
                    std::shared_ptr<value_t> v = args_[i]->evaluate();
                    auto p = std::dynamic_pointer_cast<int_value>(v);
                    params[i]->set_init_value(std::make_shared<int_node>(p->value()));
                    program_scope.current_scope().insert(new variable_int(params[i]->name(), p->value()));
                } else if (args_[i]->value_type() == variable_type::floating) {
                    std::shared_ptr<value_t> v = args_[i]->evaluate();
                    auto p = std::dynamic_pointer_cast<float_value>(v);
                    params[i]->set_init_value(std::make_shared<int_node>(static_cast<int32_t>(p->value())));
                    program_scope.current_scope().insert(new variable_int(params[i]->name(), static_cast<int32_t>(p->value())));
                } else {
                    throw_type_error("{} cannot convert to {}", args_[i]->value_type(), params[i]->value_type());
                }
            } else if (params[i]->value_type() == variable_type::floating) {
                if (args_[i]->value_type() == variable_type::integer) {
                    std::shared_ptr<value_t> v = args_[i]->evaluate();
                    auto p = std::dynamic_pointer_cast<int_value>(v);
                    params[i]->set_init_value(std::make_shared<float_node>(static_cast<float>(p->value())));
                    program_scope.current_scope().insert(new variable_float(params[i]->name(), static_cast<float>(p->value())));
                } else if (args_[i]->value_type() == variable_type::floating) {
                    std::shared_ptr<value_t> v = args_[i]->evaluate();
                    auto p = std::dynamic_pointer_cast<float_value>(v);
                    params[i]->set_init_value(std::make_shared<float_node>(p->value()));
                    program_scope.current_scope().insert(new variable_float(params[i]->name(), p->value()));
                } else {
                    throw_type_error("{} cannot convert to {}", args_[i]->value_type(), params[i]->value_type());
                }
            } else if (params[i]->value_type() == variable_type::boolean) {
                if (args_[i]->value_type() == variable_type::boolean) {
                    auto v = std::dynamic_pointer_cast<boolean_value>((args_[i]->evaluate()));
                    auto p = std::make_shared<boolean_node>(v->value());
                    params[i]->set_init_value(p);
                    program_scope.current_scope().insert(new variable_boolean(params[i]->name(), v->value()));
                } else {
                    throw_type_error("{} cannot convert to {}", variable_type::boolean, params[i]->value_type());
                }
            } else {
                throw_type_error("invalid parameter type {}", params[i]->value_type());
            }
        }

        auto [state, value] = func->body()->execute();

        program_scope.pop_scope();

        return value;
    }

    variable_type value_type() const override {
        if (name_ == "input")
            return args_[0]->value_type();
        if (name_ == "println")
            return variable_type::integer;
        auto func = static_func_decls.find(name_);
        return func->return_type();
    }

 private:
    void eatline() const {
        while (std::getchar() != '\n') {
            continue;
        }
    }

    std::string name_;
    std::vector<std::shared_ptr<expression_node>> args_;
};

class if_node : public statement_node {
 public:
    if_node(expression_node *condition, statement_node *body)
        : statement_node(ast_node_type::node_if),
          condition_(condition), if_statements_(body) {
        variable_type type = condition->value_type();
        if (type != variable_type::boolean) {
            throw_type_error(
                "condition of if statement must have a type boolean"
            );
        }
    }
    
    void set_else(std::shared_ptr<statement_node> node) {
        else_statements_ = node;
    }

    std::pair<execute_state, std::shared_ptr<value_t>> execute() override {
        std::shared_ptr<value_t> value = condition_->evaluate();
        auto v = std::dynamic_pointer_cast<boolean_value>(value);
        if (v->value()) {
            return if_statements_->execute();
        } else {
            return else_statements_->execute();
        }
    }

 private:
    std::shared_ptr<expression_node> condition_;
    std::shared_ptr<statement_node> if_statements_;
    std::shared_ptr<statement_node> else_statements_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif