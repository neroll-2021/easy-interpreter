#ifndef NEROLL_SCRIPT_DETAIL_VALUE_T_H
#define NEROLL_SCRIPT_DETAIL_VALUE_T_H

#include <string>       // std::string
#include <string_view>  // std::string_view

#include "script/detail/variable.h"

namespace neroll {

namespace script {

namespace detail {

class value_t {
 public:
    value_t(variable_type type)
        : type_(type) {}

    virtual ~value_t() {}

    variable_type type() const {
        return type_;
    }

 private:
    variable_type type_;
};

class int_value : public value_t {
 public:
    int_value(int value)
        : value_t(variable_type::integer), value_(value) {}

    int value() const {
        return value_;
    }

 private:
    int32_t value_;
};

class float_value : public value_t {
 public:
    float_value(float value)
        : value_t(variable_type::floating), value_(value) {}

    float value() const {
        return value_;
    }

 private:
    float value_;
};

class boolean_value : public value_t {
 public:
    boolean_value(bool value)
        : value_t(variable_type::boolean), value_(value) {}

    bool value() const {
        return value_;
    }

 private:
    bool value_;
};

class error_value : public value_t {
 public:
    error_value(std::string_view message)
        : value_t(variable_type::error), messaege_(message) {}

    const std::string &value() const {
        return messaege_;
    }

 private:
    std::string messaege_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif