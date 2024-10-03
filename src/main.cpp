#include <print>
#include <fstream>
#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"
#include "script/detail/variable.h"
#include "script/detail/ast.h"
#include "script/detail/ring_buffer.h"
#include "script/detail/parser.h"
int main() {
    using neroll::script::detail::token_type;
    using neroll::script::detail::variable_type;
    namespace nsd = neroll::script::detail;

    // nsd::add_node *add = new nsd::add_node(new nsd::float_node(1.2), new nsd::int_node(2));
    // nsd::multiply_node mul(add, new nsd::int_node(3));
    // std::shared_ptr<nsd::value_t> result{mul.evaluate()};
    // if (result->type() == variable_type::error) {
    //     auto error = std::dynamic_pointer_cast<nsd::error_value>(result);
    //     std::println("{}", error->value());
    // } else {
    //     switch (result->type()) {
    //         case variable_type::integer:
    //             std::println("{}", std::dynamic_pointer_cast<nsd::int_value>(result)->value());
    //             break;
    //         case variable_type::floating:
    //             std::println("{}", std::dynamic_pointer_cast<nsd::float_value>(result)->value());
    //             break;
    //         case variable_type::boolean:
    //             std::println("{}", std::dynamic_pointer_cast<nsd::boolean_value>(result)->value());
    //             break;
    //         default:
    //             std::println("error");
    //     }
    // }

    // std::ifstream fin("../../../../script/script.txt");
    // if (!fin.is_open()) {
    //     std::println("cannot open file");
    //     exit(EXIT_FAILURE);
    // }
    // neroll::script::detail::lexer<neroll::script::detail::input_stream_adapter> lexer(neroll::script::detail::input_stream_adapter{fin});
    // auto token = lexer.next_token();
    // while (token.type != token_type::end_of_input && token.type != token_type::parse_error) {
    //     std::println("{}", token);
    //     token = lexer.next_token();
    // }
    // std::println("{}", token);
}
