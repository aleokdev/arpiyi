#ifndef ARPIYI_PARSER_HPP
#define ARPIYI_PARSER_HPP

#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::codegen {

enum class EntityType {
    // parser only accepts attributes assigned to structs right now
    struct_t
};

struct Attribute {
    std::vector<std::string> arguments;
    std::string name;
    std::string scope;
};

struct AttributedEntity {
    std::vector<Attribute> attributes;
    std::string name;
    // TODO: implement scope as well
    EntityType type;
};

std::vector<AttributedEntity> parse_cpp_file(fs::path const& path);

}

#endif // ARPIYI_PARSER_HPP
