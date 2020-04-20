#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <string_view>
#include <iostream>

namespace arpiyi::codegen {

AttributedEntity parse_attributed_entity(std::string_view& view) {
    // Consume [[
    view = view.substr(2);
    std::string_view full_attribute = view.substr(0, view.find_first_of("]]"));
    std::cout << "Full attribute: " << full_attribute << std::endl;

    return {};
}

std::vector<AttributedEntity> parse_cpp_file(fs::path const& path) {
    std::vector<AttributedEntity> result;

    std::ifstream f(path);
    std::stringstream buffer;
    buffer << f.rdbuf();
    std::string_view file_view = buffer.str();

    while(!file_view.empty()) {
        if(file_view.substr(0, 2) == "[[") {
            result.emplace_back(parse_attributed_entity(file_view));
        }
        // Consume one character
        file_view = file_view.substr(1);
    }

    return result;
}

}