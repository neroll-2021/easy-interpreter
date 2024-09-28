#include <print>
#include <fstream>

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
        nsd::statement_node *node = parser.parse_block();

        assert(node != nullptr);

        assert(node->node_type() == nsd::ast_node_type::block);
        auto p = dynamic_cast<nsd::block_node *>(node);

        assert(p != nullptr);

        auto &statemetns = p->statements();

        std::println("statement count: {}", statemetns.size());

        p->execute();

        assert(nsd::program_scope.contains("a"));
        auto a = nsd::program_scope.find("a");
        auto aa = std::dynamic_pointer_cast<nsd::variable_int>(a);
        std::println("{} a = {}", nsd::variable_type_name(aa->type()), aa->value());

        assert(nsd::program_scope.contains("b"));
        auto b = nsd::program_scope.find("b");
        auto bb = std::dynamic_pointer_cast<nsd::variable_float>(b);
        std::println("{} b = {}", nsd::variable_type_name(bb->type()), bb->value());

        assert(nsd::program_scope.contains("c"));
        auto c = nsd::program_scope.find("c");
        auto cc = std::dynamic_pointer_cast<nsd::variable_boolean>(c);
        std::println("{} c = {}", nsd::variable_type_name(cc->type()), cc->value());

    } catch (std::exception &e) {
        std::println("{}", e.what());
    }
}