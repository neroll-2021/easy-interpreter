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
    std::shared_ptr<nsd::expression_node> node;
    try {
        auto p = parser.parse_expr();
        node.reset(p);
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
        return 0;
    }
    nsd::value_t *result = node->evaluate();
    if (result->type() == nsd::variable_type::error) {
        std::println("error type");
        auto p = dynamic_cast<nsd::error_value *>(result);
        std::println("{}", p->value());
        return 0;
    }
    if (result->type() == nsd::variable_type::integer) {
        auto p = dynamic_cast<nsd::int_value *>(result);
        std::println("{}", p->value());
    } else if (result->type() == nsd::variable_type::floating) {
        auto p = dynamic_cast<nsd::float_value *>(result);
        std::println("{}", p->value());
    } else if (result->type() == nsd::variable_type::boolean) {
        auto p = dynamic_cast<nsd::boolean_value *>(result);
        std::println("{}", p->value());
    }
}