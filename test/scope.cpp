#include <print>
#include "script/detail/scope.h"

namespace nsd = neroll::script::detail;

int main() {
    nsd::scope scope;
    scope.insert(new nsd::variable_int("age", 12));
    scope.insert(new nsd::variable_float("height", 155.5));
    scope.insert(new nsd::variable_boolean("coder", true));

    try {
        auto age = scope.find("age");
        auto height = scope.find("height");
        auto coder = scope.find("coder");

        std::println("{} age", nsd::variable_type_name(age->type()));
        std::println("{} height", nsd::variable_type_name(height->type()));
        std::println("{} coder", nsd::variable_type_name(coder->type()));

        std::println("");

        std::println("age: {}", std::dynamic_pointer_cast<nsd::variable_int>(age)->value());
        std::println("height: {}", std::dynamic_pointer_cast<nsd::variable_float>(height)->value());
        std::println("coder: {}", std::dynamic_pointer_cast<nsd::variable_boolean>(coder)->value());

        std::println("");

        scope.set<int32_t>("age", 10);
        scope.set<float>("height", 100.2);
        scope.set<bool>("coder", false);

        std::println("age: {}", std::dynamic_pointer_cast<nsd::variable_int>(age)->value());
        std::println("height: {}", std::dynamic_pointer_cast<nsd::variable_float>(height)->value());
        std::println("coder: {}", std::dynamic_pointer_cast<nsd::variable_boolean>(coder)->value());
    }
    catch (std::exception &e) {
        std::println("{}", e.what());
    }
    

    
}