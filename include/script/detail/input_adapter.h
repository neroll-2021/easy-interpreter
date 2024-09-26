#ifndef NEROLL_SCRIPT_DETAIL_INPUT_ADAPTER_H
#define NEROLL_SCRIPT_DETAIL_INPUT_ADAPTER_H

#include <cstdio>   // std::FILE
#include <string>   // std::char_traits
#include <istream>

namespace neroll {

namespace script {

namespace detail {

class file_input_adapter {
 public:
    using char_type = char;

    explicit file_input_adapter(std::FILE *file)
        : file_(file) {}
    
    file_input_adapter(const file_input_adapter&) = delete;
    file_input_adapter(file_input_adapter&&) noexcept = default;
    file_input_adapter& operator=(const file_input_adapter&) = delete;
    file_input_adapter& operator=(file_input_adapter&&) = delete;
    ~file_input_adapter() = default;
    
    std::char_traits<char>::int_type get_chararcter() noexcept {
        return std::fgetc(file_);
    }

 private:
    std::FILE *file_;
};

class input_stream_adapter {
 public:
    using char_type = char;

    explicit input_stream_adapter(std::istream &i)
        : is(&i), sb(i.rdbuf()) {}

    ~input_stream_adapter() {
        if (is != nullptr) {
            is->clear(is->rdstate() & std::ios::eofbit);
        }
    }

    input_stream_adapter(const input_stream_adapter&) = delete;
    input_stream_adapter& operator=(input_stream_adapter&) = delete;
    input_stream_adapter& operator=(input_stream_adapter&&) = delete;

    input_stream_adapter(input_stream_adapter &&rhs) noexcept
        : is(rhs.is), sb(rhs.sb) {
        rhs.is = nullptr;
        rhs.sb = nullptr;
    }

    std::char_traits<char>::int_type get_character() {
        auto res = sb->sbumpc();
        if (res == std::char_traits<char>::eof()) [[unlikely]] {
            is->clear(is->rdstate() | std::ios::eofbit);
        }
        return res;
    }

 private:
    std::istream *is = nullptr;
    std::streambuf *sb = nullptr;
};

}   // namespace detail

}   // namespace script

}   // namespace neroll

#endif