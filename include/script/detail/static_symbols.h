#ifndef NEROLL_SCRIPT_DETAIL_STATIC_SYMBOLS_H
#define NEROLL_SCRIPT_DETAIL_STATIC_SYMBOLS_H

#include <vector>
#include <map>
#include <stdexcept>
#include <format>
#include <optional>
#include <ranges>

#include "script/detail/variable.h"
#include "script/detail/function.h"

namespace neroll {

namespace script {

namespace detail {

class static_symbols {
 public:
    void push_scope() {
        scopes.emplace_back();
    }

    void pop_scope() {
        scopes.pop_back();
    }

    bool empty() const {
        return scopes.empty();
    }

    void insert(std::string_view var_name, variable_type type) {
        scopes.back().insert({std::string{var_name}, type});
    }

    const std::map<std::string, variable_type, std::less<>> current_scope() const {
        return scopes.back();
    }

    std::optional<std::pair<std::string, variable_type>> find(std::string_view name) const {
        for (const auto &scope : scopes | std::views::reverse) {
            auto iter = scope.find(name);
            if (iter != scope.end()) {
                return *iter;
            }
        }
        return std::nullopt;

    }

 private:
    std::vector<std::map<std::string, variable_type, std::less<>>> scopes;
};

inline function_declaration static_func_decls;

inline static_symbols static_symbol_table;

}

}

}

#endif