#ifndef NEROLL_SCRIPT_DETAIL_FUNCTION_H
#define NEROLL_SCRIPT_DETAIL_FUNCTION_H

#include <string>           // std::string
#include <string_view>      // std::string_view
#include <vector>           // std::vector
#include <map>              // std::map
#include <memory>           // std::unique_ptr
#include <compare>

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

class func_decl_node;
class statement_node;

class function_declaration {
 public:
    bool contains(std::string_view name) const {
        return functions_.contains(name);
    }

    func_decl_node *find(std::string_view name) {
        auto iter = functions_.find(name);
        if (iter == functions_.end())
            return nullptr;
        return iter->second;
    }

    void add(std::string_view name, func_decl_node *node) {
        functions_.insert({std::string{name}, node});
    }

 private:
    std::map<std::string, func_decl_node *, std::less<>> functions_;
};

// class function {
//  public:
//     function(std::string_view name, variable_type type, std::vector<variable_type> &&param)
//         : name_(name), return_type_(type), params_(std::move(param)) {}

//     std::strong_ordering operator<=>(const function &rhs) const = default;
//     bool operator==(const function &rhs) const = default;

//  private:
//     std::string name_;
//     variable_type return_type_;
//     std::vector<variable_type> params_;
// };

inline function_declaration func_decls;

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif