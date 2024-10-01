#include <print>
#include <fstream>

#include "script/detail/input_adapter.h"
#include "script/detail/new_parser.h"

namespace nsd = neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    nsd::parser parser{nsd::lexer<nsd::input_stream_adapter>{nsd::input_stream_adapter{fin}}};
    try {
        std::shared_ptr<nsd::expression_node> node{parser.parse_equal()};
        assert(node != nullptr);

        std::shared_ptr<nsd::value_t> value{node->evaluate()};
        assert(value != nullptr);

        switch (value->type()) {
            case nsd::variable_type::integer: {
                auto p = std::dynamic_pointer_cast<nsd::int_value>(value);
                assert(p != nullptr);
                std::println("{}", p->value());
            }
            break;
            case nsd::variable_type::floating: {
                auto p = std::dynamic_pointer_cast<nsd::float_value>(value);
                assert(p != nullptr);
                std::println("{}", p->value());
            }
            break;
            case nsd::variable_type::boolean: {
                auto p = std::dynamic_pointer_cast<nsd::boolean_value>(value);
                assert(p != nullptr);
                std::println("{}", p->value());
            }
            break;
            case nsd::variable_type::function:
                std::println("invalid function type");
                break;
            case nsd::variable_type::error:
                std::println("error type");
                break;
            default:
                std::println("unknown type");
                break;
        }
        
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
    }
}