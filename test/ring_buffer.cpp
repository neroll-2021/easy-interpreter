#include <print>
#include "script/detail/ring_buffer.h"
namespace nsd = neroll::script::detail;
int main() {
    nsd::ring_buffer<int> buffer(10);
    std::println("capacity: {}", buffer.capacity());
    for (std::size_t i = 0; i < buffer.capacity(); i++) {
        buffer.add(static_cast<int>(i));
    }
    for (std::size_t i = 0; i < buffer.capacity(); i++) {
        std::println("next {}: {}", i, buffer.get_next(i));
    }
}