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

namespace neroll {

namespace script {

namespace detail {

enum class ast_node_type {
    function, node_for, node_if, node_while, integer, floating, boolean, add, binary, unary,
    declaration, block
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

enum class execute_state {
    normal, broken, continued, returned
};

class statement_node : public ast_node {
 public:
    statement_node(ast_node_type node_type)
        : ast_node(node_type) {}
    
    virtual ~statement_node() {}

    virtual execute_state execute() = 0;
};

// 
variable_type binary_expression_type(variable_type lhs_type, token_type op, variable_type rhs_type) {
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

class negative_node : public unary_node {
 public:
    negative_node(expression_node *value)
        : unary_node(value) {
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

class declaration_node : public statement_node {
 public:
    declaration_node(variable_type type, std::string_view name, expression_node *value)
        : statement_node(ast_node_type::declaration), type_(type), variable_name_(name), init_value_(value) {
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

    execute_state execute() override {
        // program_scope.current_scope().insert()
        switch (type_) {
            case variable_type::integer: {
                auto p = std::dynamic_pointer_cast<int_node>(init_value_);
                value_t *r = p->evaluate();
                auto v = dynamic_cast<int_value *>(r);
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
        return execute_state::normal;
    }

 private:
    variable_type type_;
    std::string variable_name_;
    std::shared_ptr<expression_node> init_value_;
};


class block_node : public statement_node {
 public:
    block_node() : statement_node(ast_node_type::block) {}

    void insert(statement_node *statement) {
        statements_.emplace_back(statement);
    }

    const std::vector<std::shared_ptr<statement_node>> &statements() const {
        return statements_;
    }

    execute_state execute() override {
        for (auto &statement : statements_) {
            execute_state result = statement->execute();
            if (result != execute_state::normal) {
                return result;
            }
        }
        return execute_state::normal;
    }

 private:
    std::vector<std::shared_ptr<statement_node>> statements_;
};

class for_node : public statement_node {
 public:
    for_node() : statement_node(ast_node_type::node_for) {}

    execute_state execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        init_statement_->evaluate();
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
            execute_state state = statements_->execute();
            if (state == execute_state::continued) {
                update_->evaluate();
                continue;
            } else if (state == execute_state::broken) {
                break;
            } else if (state == execute_state::returned) {
                return execute_state::returned;
            }
            update_->evaluate();
        }
        return execute_state::normal;
    }

 private:
    std::shared_ptr<expression_node> init_statement_;
    std::shared_ptr<expression_node> condition_;
    std::shared_ptr<expression_node> update_;
    std::shared_ptr<block_node> statements_;
};

class while_node : statement_node {
 public:
    while_node() : statement_node(ast_node_type::node_while) {}

    execute_state execute() override {
        assert(condition_->value_type() == variable_type::boolean);
        while (dynamic_cast<boolean_value *>(condition_->evaluate())->value()) {
            execute_state state = statements_->execute();
            if (state == execute_state::continued) {
                continue;
            } else if (state == execute_state::broken) {
                break;
            } else if (state == execute_state::returned) {
                return execute_state::returned;
            }
        }
        return execute_state::normal;
    }
 private:
    std::shared_ptr<expression_node> condition_;
    std::shared_ptr<block_node> statements_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif