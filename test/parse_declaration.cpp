#include <print>
#include <fstream>
#include <cassert>
#include "script/detail/parser.h"

namespace nsd = neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    nsd::parser<nsd::input_stream_adapter> parser{nsd::lexer<nsd::input_stream_adapter>{nsd::input_stream_adapter{fin}}};

    try {
        nsd::statement_node *node = parser.parse_declaration();
        node->execute();

        std::println("{}", nsd::program_scope.contains("neroll"));

        auto var = nsd::program_scope.find("neroll");

        std::println("variable name: {}", var->name());
        std::println("variable type: {}", nsd::variable_type_name(var->type()));
        assert(var->type() == nsd::variable_type::integer);
        std::println("variable value: {}", std::dynamic_pointer_cast<nsd::variable_int>(var)->value());
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
    }
}

