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

        auto root = std::dynamic_pointer_cast<nsd::block_node>(node);
        assert(root != nullptr);
        assert(root->statements().size() == 2);

        assert(root->statements()[0]->node_type() == nsd::ast_node_type::func_decl);
        assert(root->statements()[1]->node_type() == nsd::ast_node_type::declaration);

        auto left = root->statements()[0];
        auto func_decl = std::dynamic_pointer_cast<nsd::func_decl_node>(left);
        assert(func_decl != nullptr);
        
        auto body = std::dynamic_pointer_cast<nsd::block_node>(func_decl->body());
        assert(body != nullptr);
        assert(body->statements().size() == 1);
        assert(body->statements()[0]->node_type() == nsd::ast_node_type::jump);



        std::pair<nsd::execute_state, nsd::value_t *> result = node->execute();
        std::println("execute complete");
        auto [state, value] = result;

        assert(state == nsd::execute_state::normal);

        std::println("done");

    }
    catch (std::runtime_error &e) {
        std::println("execute error: {}", e.what());
        return 0;
    }
}