#ifndef NEROLL_SCRIPT_DETAIL_FUNCTION_H
#define NEROLL_SCRIPT_DETAIL_FUNCTION_H

#include <string>           // std::string
#include <string_view>      // std::string_view
#include <vector>           // std::vector
#include <map>              // std::map
#include <memory>           // std::unique_ptr

#include "script/detail/variable.h"     // variable_type


namespace neroll {

namespace script {

namespace detail {

// class function {

//  private:
//     std::string name_;
//     variable_type return_type_;
//     std::unordered_map<std::string_view, std::unique_ptr<variable>> variables_;
//     std::vector<variable_type> parameters_;
// };

class statement_node;

class function_declaration {
 public:
    bool contains(std::string_view name) const {
        return functions_.contains(name);
    }

    statement_node *find(std::string_view name) {
        auto iter = functions_.find(name);
        if (iter == functions_.end())
            return nullptr;
        return iter->second;
    }

    void add(std::string_view name, statement_node *node) {
        functions_.insert({std::string{name}, node});
    }

 private:
    std::map<std::string, statement_node *, std::less<>> functions_;
};

inline function_declaration func_decls;

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif