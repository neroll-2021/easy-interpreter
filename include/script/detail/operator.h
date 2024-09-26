#ifndef NEROLL_SCRIPT_DETAIL_OPERATOR_H
#define NEROLL_SCRIPT_DETAIL_OPERATOR_H

namespace neroll {

namespace script {

namespace detail {

class plus {
 public:
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs + rhs;
    }
};

class minus {
 public:
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs - rhs;
    }
};

class multiplies {
 public:
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs * rhs;
    }
};


class divides {
 public:
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs / rhs;
    }
};

class modulus {
 public:
    template <typename T, typename U>
    constexpr auto operator()(const T &lhs, const U &rhs) {
        return lhs % rhs;
    }
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif