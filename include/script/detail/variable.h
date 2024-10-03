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
    error = -1, integer, floating, boolean, function
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

variable_type arithmetic_type_cast(variable_type lhs, variable_type rhs) {
    assert(static_cast<std::size_t>(lhs) < 4);
    assert(static_cast<std::size_t>(rhs) < 4);
    static constexpr std::array<std::array<int, 4>, 4> table{{{0, 1, -1, -1}, {1, 1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}}};
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

template <>
struct std::formatter<neroll::script::detail::variable_type> {
    constexpr auto parse(std::format_parse_context &context) {
        if (context.begin() != context.end())
            throw std::format_error("invalid format specifier");
        return context.begin();
    }

    auto format(neroll::script::detail::variable_type type, std::format_context &context) const {
        using neroll::script::detail::variable_type_name;
        return std::format_to(context.out(), "{}", variable_type_name(type));
    }
};

#endif