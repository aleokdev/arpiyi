#include <cppast/cpp_file.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <rapidjson/document.h>
#include <rapidjson/reader.h>

namespace fs = std::filesystem;

using libclang_parse_result = std::unique_ptr<cppast::cpp_file>;

libclang_parse_result parse_file(fs::path const& path,
                                 cppast::libclang_compile_config const& config) {
    cppast::libclang_parser parser;
    cppast::cpp_entity_index root_index;
    try {
        return parser.parse(root_index, path.generic_string(), config);
    } catch (cppast::libclang_error const& e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }
}

std::vector<libclang_parse_result> parse_directory(fs::path const& path,
                                                   cppast::libclang_compile_config const& config) {
    std::vector<libclang_parse_result> results;
    if (!fs::is_directory(path)) {
        return results;
    }

    for (auto const& entry : fs::directory_iterator(path)) {
        // Only parse headers right now
        if (entry.path().extension() == ".hpp") {
            results.emplace_back(parse_file(entry.path(), config));
        }
    }
    return results;
}

std::vector<fs::path> extract_include_dirs(std::string const& compile_command) {
    std::vector<fs::path> include_dirs;
    std::istringstream cmdss(compile_command);
    for (std::string arg; std::getline(cmdss, arg, ' ');) {
        if (arg.substr(0, 2) == "-I") {
            include_dirs.emplace_back(arg.substr(2));
        }
    }

    return include_dirs;
}

std::vector<fs::path> extract_include_dirs(fs::path const& database_path,
                                           fs::path const& project_bin_dir) {
    std::vector<fs::path> include_dirs;
    rapidjson::Document database;
    {
        std::ifstream f(database_path);
        std::stringstream buffer;
        buffer << f.rdbuf();

        database.Parse(buffer.str().data());
    }

    for (const auto& compile_target : database.GetArray()) {
        const fs::path compile_target_path = compile_target.GetObject()["directory"].GetString();

        if (fs::equivalent(compile_target_path, project_bin_dir)) {
            return extract_include_dirs(std::string(compile_target.GetObject()["command"].GetString()));
        }
    }

    std::cerr << "Could not find any include directories in database." << std::endl;
    return include_dirs;
}

int main(int argc, const char* argv[]) try {
    if (argc < 2) {
        std::cerr << "Please supply a valid binary folder." << std::endl;
        return -1;
    }
    const fs::path assets_path = fs::absolute(fs::path("shared/include/assets"));
    const fs::path database_path = fs::path(argv[1]) / "compile_commands.json";
    std::cout << "Starting codegen with folder = " << assets_path.generic_string()
              << " and database = " << database_path << std::endl;

    cppast::libclang_compile_config config;
    for (const auto& include_dir : extract_include_dirs(database_path, fs::path("build/shared"))) {
        std::cout << "Include dir: " << include_dir.generic_string() << std::endl;
        config.add_include_dir(include_dir.generic_string());
    }

    config.set_flags(cppast::cpp_standard::cpp_1z);

    const auto result_files = parse_directory(assets_path, config);
    std::cout << "Processing " << result_files.size() << " files." << std::endl;
    for (const auto& result : result_files) {
        if (result == nullptr)
            continue;

        cppast::visit(*result, [](cppast::cpp_entity const& e, cppast::visitor_info info) -> bool {
            if (e.kind() == cppast::cpp_entity_kind::file_t || cppast::is_templated(e) ||
                cppast::is_friended(e)) {
                // no need to do anything for a file,
                // templated and friended entities are just proxies, so skip those as well
                // return true to continue visit for children
                return true;
            } else {
                if (!e.attributes().empty()) {
                    std::cout << e.name() << " has " << e.attributes().size() << " attributes."
                              << std::endl;
                }
            }
            return true;
        });
    }
} catch (const cppast::libclang_error& ex) {
    std::cerr << std::string("[fatal parsing error] ") + ex.what() << std::endl;
    return -1;
} catch (...) {
    std::cerr << "Unknown error!" << std::endl;
    return -1;
}