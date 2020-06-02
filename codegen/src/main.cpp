#include "parser.hpp"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace arpiyi::codegen;

struct AssetWithDirName {
    AttributedEntity asset_entity;
    Attribute dir_name_attribute;
};

struct SerializableAsset {
    fs::path header_path;
    AttributedEntity asset_entity;
    /// Names of asset types that must be loaded before this one.
    std::vector<std::string> asset_load_dependencies;
};

void create_assets_codegen_file(std::vector<AssetWithDirName> const& assets_with_dir_name,
                                fs::path const& build_folder) {
    const fs::path assets_out_path = build_folder /"shared/include/assets/asset_cg.hpp";
    fs::create_directories(assets_out_path.parent_path());
    auto out_f = std::ofstream(assets_out_path);
    out_f << "// asset_cg.hpp\n"
          << "// Generated header for usage with the arpiyi shared library.\n"
          << "#ifndef ARPIYI_ASSET_CG_HPP\n"
          << "#define ARPIYI_ASSET_CG_HPP\n\n"
          << "#include <string_view>\n\n"
          << "namespace arpiyi::assets {\n";

    for (const auto& asset_e : assets_with_dir_name) {
        const auto asset_dir_name = asset_e.dir_name_attribute.arguments[0];
        out_f << "struct " << asset_e.asset_entity.name << ";\n"
              << "template<> struct AssetDirName<" << asset_e.asset_entity.name
              << "> { constexpr static std::string_view value = " << asset_dir_name << "; };\n\n";
    }

    out_f << "}\n"
          << "#endif // ARPIYI_ASSET_CG_HPP" << std::endl;

    std::cout << "Assets file written to " << assets_out_path << std::endl;
}

void create_serializer_codegen_file(std::vector<SerializableAsset> serializable_assets, fs::path const& build_folder) {
    const fs::path serializer_out_path = build_folder / "shared/src/serializer_cg.cpp";
    fs::create_directories(serializer_out_path.parent_path());
    auto out_f = std::ofstream(serializer_out_path);
    out_f << "// serializer_cg.cpp\n"
             "// Generated source file for usage with the arpiyi shared library.\n"
             "#include <functional>\n"
             "#include <filesystem>\n\n"
             "#include \"serializer.hpp\"\n\n"
             "namespace fs = std::filesystem;\n\n";
    // Include asset header paths
    for (const auto& asset : serializable_assets) {
        out_f << "#include \"" << asset.header_path.generic_string() << "\"\n";
    }

    /* clang-format off */
    out_f
        << "\nnamespace arpiyi::serializer {\n\n"
           "/// The number of asset structs that are marked with the [[assets::serialize]] \n"
           "/// attribute.\n"
           "const std::size_t serializable_assets = " << serializable_assets.size() << ";\n\n"

           "/// Loads one single asset type from a project folder and places the results in the \n"
           "/// corresponding asset container. The \"One single asset type\" is useful for \n"
           "/// simulating coroutines.\n"
           "/// You can call this function with indices from 0 to <serializable_assets> to load \n"
           "/// all assets.\n"
           "void load_one_asset_type(std::size_t asset_type_index, fs::path project_path,\n"
           "                 std::function<void(std::string_view /* progress string */, "
           "float /* progress (0~1) */)>\n"
           "                     per_step_func) {\n"
           "\tswitch(asset_type_index) {\n";
    /* clang-format on */

    std::vector<SerializableAsset> loaded_assets;

    const auto load_asset = [&out_f, &loaded_assets](SerializableAsset const& asset) {
        /* clang-format off */
        out_f << "\t\tcase " << loaded_assets.size() << ": "
                 "load_assets<assets::" << asset.asset_entity.name << ">"
                 "(project_path, per_step_func); break;\n";
        /* clang-format on */
        loaded_assets.emplace_back(asset);
    };

    while (loaded_assets.size() != serializable_assets.size()) {
        for (const auto& asset : serializable_assets) {
            if (std::find_if(loaded_assets.begin(), loaded_assets.end(),
                             [&asset](const auto& other) -> bool {
                                 return asset.asset_entity.name == other.asset_entity.name;
                             }) != loaded_assets.end())
                continue;
            if (asset.asset_load_dependencies.empty())
                load_asset(asset);
            else {
                // Use lambda so we can use return
                const auto check_dependencies = [&]() {
                    for (const auto& dependency : asset.asset_load_dependencies) {
                        if (std::find_if(loaded_assets.begin(), loaded_assets.end(),
                                         [&dependency](const auto& other) -> bool {
                                             return dependency == other.asset_entity.name;
                                         }) == loaded_assets.end())
                            return;
                    }
                    load_asset(asset);
                };

                check_dependencies();
            }
        }
    }

    out_f
        << "\t}\n}\n\n"
           "/// Saves one single asset type from its container to a project folder.\n"
           "/// The \"One single asset type\" is useful for simulating coroutines.\n"
           "/// You can call this function with indices from 0 to <serializable_assets> to save \n"
           "/// all assets.\n"
           "void save_one_asset_type(std::size_t asset_type_index, fs::path project_path,\n"
           "                 std::function<void(std::string_view /* progress string */, "
           "float /* progress (0~1) */)>\n"
           "                     per_step_func) {\n"
           "\tswitch(asset_type_index) {\n";

    std::size_t i = 0;
    for (const auto& e : serializable_assets) {
        /* clang-format off */
        out_f << "\t\tcase " << i << ": "
                 "save_assets<assets::" << e.asset_entity.name << ">"
                 "(project_path, per_step_func); break;\n";
        /* clang-format on */
        ++i;
    }

    out_f << "\t}\n}\n}\n";

    std::cout << "Assets file written to " << serializer_out_path << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "You need to supply the project build path." << std::endl;
        return -1;
    }
    const fs::path assets_path = fs::absolute(fs::path("shared/include/assets"));
    const fs::path build_path = fs::absolute(fs::path(argv[1]));
    std::cout << "Starting codegen with folder = " << assets_path.generic_string() << std::endl;

    std::vector<AssetWithDirName> assets_with_dir_name;
    std::vector<SerializableAsset> serializable_assets;

    for (auto const& entry : fs::directory_iterator(assets_path)) {
        const auto entities = arpiyi::codegen::parse_cpp_file(entry.path());
        for (const auto& e : entities) {
            SerializableAsset serializable_asset;
            std::vector<std::string> load_dependencies;
            bool do_serialize = false;

            for (const auto& attr : e.attributes) {
                if (attr.scope == "meta") {
                    if (attr.name == "dir_name") {
                        assert(!attr.arguments.empty());
                        assets_with_dir_name.emplace_back(AssetWithDirName{e, attr});
                    } else {
                        std::cerr << "Unrecognized attribute name: meta::" << attr.name
                                  << std::endl;
                    }
                } else if (attr.scope == "assets") {
                    if (attr.name == "serialize") {
                        do_serialize = true;
                    } else if (attr.name == "load_before") {
                        load_dependencies.emplace_back(attr.arguments[0]);
                    } else {
                        std::cerr << "Unrecognized attribute name: assets::" << attr.name
                                  << std::endl;
                    }
                }
            }

            if (do_serialize) {
                serializable_assets.emplace_back(SerializableAsset{
                    fs::relative(entry.path(), "shared/include"), e, load_dependencies});
            }
        }
    }

    create_assets_codegen_file(assets_with_dir_name, build_path);
    create_serializer_codegen_file(serializable_assets, build_path);
}