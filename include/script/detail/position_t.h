#ifndef NEROLL_DETAIL_POSITION_T_H
#define NEROLL_DETAIL_POSITION_T_H

#include <cstddef>  // std::size_t

namespace neroll {

namespace script {

namespace detail {

struct position_t {
    std::size_t chars_read_total = 0;
    std::size_t chars_read_current_line = 0;
    std::size_t lines_read = 0;
};

}

}

}

#endif