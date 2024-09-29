#include <print>
#include <fstream>
#include "script/detail/input_adapter.h"
#include "script/detail/lexer.h"

using namespace neroll::script::detail;

int main() {
    std::ifstream fin("../../../../script/script.txt");
    if (!fin.is_open()) {
        std::println("cannot open file");
        return 0;
    }
    lexer<input_stream_adapter> lex{input_stream_adapter(fin)};

    token t = lex.next_token();
    while (t.type != token_type::parse_error && t.type != token_type::end_of_input) {
        std::println("{}", t);
        t = lex.next_token();
    }
    lex.rewind();
    t = lex.next_token();
    while (t.type != token_type::parse_error && t.type != token_type::end_of_input) {
        std::println("{}", t);
        t = lex.next_token();
    }
}