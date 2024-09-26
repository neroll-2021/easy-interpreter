#ifndef NEROLL_SCRIPT_DETAIL_FUNCTION_H
#define NEROLL_SCRIPT_DETAIL_FUNCTION_H

#include <string>           // std::string
#include <string_view>      // std::string_view
#include <vector>           // std::vector
#include <unordered_map>    // std::unrodered_map
#include <memory>           // std::unique_ptr

#include "script/detail/variable.h"     // variable_type


namespace neroll {

namespace script {

namespace detail {

class function {

 private:
    std::string name_;
    variable_type return_type_;
    std::unordered_map<std::string_view, std::unique_ptr<variable>> variables_;
    std::vector<variable_type> parameters_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif