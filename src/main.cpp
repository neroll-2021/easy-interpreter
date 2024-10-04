#include <print>
#include <fstream>
#include "script/detail/parser.h"
int main() {
    using neroll::script::detail::token_type;
    using neroll::script::detail::variable_type;
    namespace nsd = neroll::script::detail;

    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    nsd::parser<nsd::input_stream_adapter> parser{nsd::lexer<nsd::input_stream_adapter>{nsd::input_stream_adapter{fin}}};
    std::shared_ptr<nsd::statement_node> node;

    try {
        node.reset(parser.parse());
    }
    catch (std::exception &e) {
        std::println("parse error: {}", e.what());
        return 0;
    }

    try {
        assert(node != nullptr);

        std::pair<nsd::execute_state, std::shared_ptr<nsd::value_t>> result = node->execute();

        auto [state, value] = result;

        assert(state == nsd::execute_state::normal);

    }
    catch (std::exception &e) {
        std::println("execute error: {}", e.what());
    }
}
