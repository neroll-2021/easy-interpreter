#include <print>
#include <fstream>

#include "script/detail/syntax_checker.h"

namespace nsd = neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    nsd::syntax_checker checker{nsd::lexer<nsd::input_stream_adapter>{nsd::input_stream_adapter{fin}}};

    try {
        checker.check();
    }
    catch (std::runtime_error &e) {
        std::println("{}", e.what());
    }
}