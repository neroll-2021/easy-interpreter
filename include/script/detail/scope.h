#ifndef NEROLL_SCRIPT_DETAIL_SCOPE_H
#define NEROLL_SCRIPT_DETAIL_SCOPE_H

#include <string_view>      // std::string_view
#include <unordered_map>    // std::unordered_map
#include <memory>           // std::shared_ptr
#include <vector>           // std::vector
#include <ranges>

#include "script/detail/variable.h"

namespace neroll {

namespace script {

namespace detail {


/**
 * store variable, no function
 * 
 */
class scope {
 public:
    bool contains(std::string_view variable_name) const {
        return variables_.contains(variable_name);
    }

    void insert(variable *var) {
        auto p = variables_.insert({var->name(), std::shared_ptr<variable>(var)});
    }

    std::shared_ptr<variable> find(std::string_view name) const {
        auto iter = variables_.find(name);
        if (iter == variables_.end())
            return nullptr;
        return iter->second;
    }

    template <typename T>
    void set(std::string_view name, const T &value) {
        std::shared_ptr<variable> var = find(name);
        switch (var->type()) {
            case variable_type::integer: {
                if (!std::is_same_v<T, int32_t>) {
                    throw std::runtime_error("type not compatible");
                }
                auto p = std::dynamic_pointer_cast<variable_int>(var);
                p->set_value(static_cast<int32_t>(value));
                break;
            }
            case variable_type::floating: {
                if (!std::is_same_v<T, float>) {
                    throw std::runtime_error("type not compatible");
                }
                auto p = std::dynamic_pointer_cast<variable_float>(var);
                p->set_value(static_cast<float>(value));
                break;
            }
            case variable_type::boolean: {
                if (!std::is_same_v<T, bool>) {
                    throw std::runtime_error("type not compatible");
                }
                auto p = std::dynamic_pointer_cast<variable_boolean>(var);
                p->set_value(static_cast<bool>(value));
                break;
            }
            case variable_type::function:
                throw std::runtime_error("variable cannot be function");
            default:
                throw std::runtime_error("error variable type");
        }
    }

 private:
    std::unordered_map<std::string_view, std::shared_ptr<variable>> variables_;
};

class scope_chain {
 public:
    scope_chain() {
        // global scope
        scopes_.emplace_back();
    }

    void push_scope() {
        scopes_.emplace_back();
    }

    void pop_scope() {
        scopes_.pop_back();
    }

    scope &current_scope() {
        return scopes_.back();
    }

    bool contains(std::string_view name) const {
        for (const auto &scope : scopes_ | std::views::reverse) {
            if (scope.contains(name))
                return true;
        }
        return false;
    }

    std::shared_ptr<variable> find(std::string_view name) const {
        for (const auto &scope : scopes_ | std::views::reverse) {
            std::shared_ptr<variable> var = scope.find(name);
            if (var != nullptr)
                return var;
        }
        return nullptr;
    }

    void insert(variable *var) {
        scopes_.back().insert(var);
    }

    template <typename T>
    void set(std::string_view name, const T &value) {
        for (const auto &scope : scopes_ | std::views::reverse) {
            if (scope.contains(name))
                scope.set(name, value);
        }
    }

 private:
    std::vector<scope> scopes_;
};

inline scope_chain program_scope;

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif