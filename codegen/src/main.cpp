#include "parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <cassert>

namespace fs = std::filesystem;

int main() {
    const fs::path assets_path = fs::absolute(fs::path("shared/include/assets"));
    std::cout << "Starting codegen with folder = " << assets_path.generic_string() << std::endl;

    std::stringstream codegen_out;
    codegen_out << "// asset_cg.hpp\n";
    codegen_out << "// Generated header for usage with the arpiyi shared library.\n";
    codegen_out << "#ifndef ARPIYI_ASSET_CG_HPP\n";
    codegen_out << "#define ARPIYI_ASSET_CG_HPP\n";
    codegen_out << "namespace arpiyi::assets {\n";

    // Create file now so we don't get "asset_cg.hpp doesn't exist" errors when parsing
    const fs::path out_path = "build/shared/include/assets/asset_cg.hpp";
    fs::create_directories(out_path.parent_path());
    auto out_f = std::ofstream(out_path);
    std::vector<arpiyi::codegen::AttributedEntity> serialized_assets;

    for (auto const& entry : fs::directory_iterator(assets_path)) {
        const auto entities = arpiyi::codegen::parse_cpp_file(entry.path());
        for(const auto& e : entities) {
            for(const auto& attr : e.attributes) {
                if (attr.scope == "meta") {
                    if (attr.name == "dir_name") {
                        assert(!attr.arguments.empty());
                        const auto asset_dir_name = attr.arguments[0];
                        codegen_out << "struct " << e.name << ";\n";
                        codegen_out << "template<> struct AssetDirName<" << e.name
                                    << "> { constexpr static std::string_view value = "
                                    << asset_dir_name << "; };\n\n";
                    } else {
                        std::cerr << "Unrecognized attribute name: meta::" << attr.name
                                  << std::endl;
                    }
                } else if (attr.scope == "assets") {
                    if (attr.name == "serialize") {
                        serialized_assets.emplace_back(e);
                    } else {
                        std::cerr << "Unrecognized attribute name: assets::" << attr.name
                                  << std::endl;
                    }
                }
            }
        }
    }

    codegen_out << "}\n\n";
    codegen_out << "#include \"serializer.hpp\"\n\n";
    codegen_out << "namespace arpiyi::serializer {\n";
    codegen_out << "void load_all_assets(fs::path project_path,\n"
                   "                 std::function<void(std::string_view /* progress string */, float /* progress (0~1) */)>\n"
                   "                     per_step_func) {\n";

    for(const auto& e : serialized_assets) {
        codegen_out << "\tload_assets<assets::" << e.name << ">(project_path, per_step_func);\n";
    }

    codegen_out << "}\n}\n";
    codegen_out << "#endif // ARPIYI_ASSET_CG_HPP";

    out_f << codegen_out.str() << std::endl;
    std::cout << "Assets file written to " << out_path << std::endl;
}