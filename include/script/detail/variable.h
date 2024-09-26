#ifndef NEROLL_SCRIPT_DETAIL_VARIABLE_H
#define NEROLL_SCRIPT_DETAIL_VARIABLE_H

#include <string>       // std::string
#include <string_view>  // std::string_view
#include <compare>      // std::weak_ordering
#include <cstdint>      // int32_t
#include <array>        // std::array
#include <cassert>      // assert

namespace neroll {

namespace script {

namespace detail {

enum class variable_type {
    integer, floating, boolean, error
};

const char *variable_type_name(variable_type type) {
    switch (type) {
        case variable_type::integer:
            return "int";
        case variable_type::floating:
            return "float";
        case variable_type::boolean:
            return "boolean";
        default:
            return "error";
    }
}

variable_type variable_type_cast(variable_type lhs, variable_type rhs) {
    assert(static_cast<std::size_t>(lhs) < 3);
    assert(static_cast<std::size_t>(rhs) < 3);
    static constexpr std::array<std::array<int, 3>, 3> table{{{0, 1, 3}, {1, 1, 3}, {3, 3, 3}}};
    std::size_t lhs_index = static_cast<std::size_t>(lhs);
    std::size_t rhs_index = static_cast<std::size_t>(rhs);
    return static_cast<variable_type>(table[lhs_index][rhs_index]);
}

class variable {
 public:
    variable(std::string_view name, variable_type type)
        : name_(name), type_(type) {}
    
    virtual ~variable() = default;

    const std::string &name() const {
        return name_;
    }
    variable_type type() const {
        return type_;
    }

    std::weak_ordering operator<=>(const variable &rhs) const {
        return name_ <=> rhs.name_;
    }

    bool operator==(const variable &rhs) const = default;

 private:
    std::string name_;
    variable_type type_;
};

class variable_int : public variable {
 public:
    variable_int(std::string_view name, int32_t value)
        : variable(name, variable_type::integer), value_(value) {}
    
    int32_t value() const {
        return value_;
    }
    void set_value(int32_t value) {
        value_ = value;
    }

 private:
    int32_t value_;
};

class variable_float : public variable {
 public:
    variable_float(std::string_view name, float value)
        : variable(name, variable_type::floating), value_(value) {}
    
    float value() const {
        return value_;
    }
    void set_value(float value) {
        value_ = value;
    }

 private:
    float value_;
};

class variable_boolean : public variable {
 public:
    variable_boolean(std::string_view name, bool value)
        : variable(name, variable_type::boolean), value_(value) {}
    
    bool value() const {
        return value_;
    }
    void set_value(bool value) {
        value_ = value;
    }

 private:
    bool value_;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif