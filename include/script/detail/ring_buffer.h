#ifndef NEROLL_SCRIPT_DETAIL_RING_BUFFER_H
#define NEROLL_SCRIPT_DETAIL_RING_BUFFER_H

#include <cstddef>  // std::size_t
#include <memory>   // std::unique_ptr

namespace neroll {

namespace script {

namespace detail {

template <typename T>
class ring_buffer {
 public:
    ring_buffer(std::size_t capacity)
        : capacity_(capacity),
          data_(std::make_unique<T[]>(capacity)) {}
    
    std::size_t capacity() const {
        return capacity_;
    }
    
    void add(const T &value) {
        data_[pos_] = value;
        pos_ = (pos_ + 1) % capacity_;
    }

    T get(std::size_t k) const {
        return data_[(pos_ + k - 1) % capacity_];
    }

 private:
    std::size_t capacity_{};
    std::size_t pos_{};
    std::unique_ptr<T []> data_{};
};


}   // neroll detail

}   // neroll script

}   // namespace neroll

#endif