#include <print>
#include <fstream>

#include "script/detail/input_adapter.h"
#include "script/detail/parser.h"

namespace nsd = neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    nsd::parser<nsd::input_stream_adapter> parser{nsd::lexer<nsd::input_stream_adapter>{nsd::input_stream_adapter{fin}}};
    std::shared_ptr<nsd::statement_node> node;

    try {
        std::println("parse begin");
        node.reset(parser.parse_program());
        std::println("parse end");
    }
    catch (std::exception &e) {
        std::println("parse error: {}", e.what());
        return 0;
    }

    try {
        assert(node != nullptr);

        std::println("execute begin");

        std::pair<nsd::execute_state, std::shared_ptr<nsd::value_t>> result = node->execute();
        std::println("execute complete");
        auto [state, value] = result;

        assert(state == nsd::execute_state::normal);

        std::println("done");

    }
    catch (std::exception &e) {
        std::println("execute error: {}", e.what());
        return 0;
    }
}