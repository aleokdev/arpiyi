#include "serializer.hpp"

#include "noc_file_dialog.h"

#include <fstream>
#include <sstream>

#include "map_manager.hpp"
#include "project_info.hpp"
#include "tileset_manager.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi_editor::serializer {
namespace meta_file_definitions {

/* clang-format off */
template<> struct AssetDirName<assets::Map> {
    constexpr static std::string_view value = "maps";
};

template<> struct AssetDirName<assets::Texture> {
    constexpr static std::string_view value = "textures";
};

template<> struct AssetDirName<assets::Tileset> {
    constexpr static std::string_view value = "tilesets";
};
/* clang-format on */

} // namespace meta_file_definitions

static void save_project_file(fs::path base_dir) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace project_file_definitions;

    w.StartObject();
    {
        w.Key(editor_version_json_key.data());
        w.String(ARPIYI_EDITOR_VERSION);
    }
    w.EndObject();

    {
        std::ofstream project_file(base_dir / "project.json");
        project_file << s.GetString();
    }
}

struct ProjectFileData {
    std::string editor_version;
};

static ProjectFileData load_project_file(fs::path base_dir) {
    std::ifstream f(base_dir / "project.json");
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    ProjectFileData file_data;

    using namespace project_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        }
    }

    return file_data;
}

void save_project(fs::path dir) {
    save_project_file(dir);
    save_asset_files<assets::Tileset>(dir);
    save_asset_files<assets::Map>(dir);
    save_asset_files<assets::Texture>(dir);
}

void load_project(fs::path dir) {
    ProjectFileData file_data(load_project_file(dir));

    if (ARPIYI_EDITOR_VERSION != file_data.editor_version) {
        // TODO: do something if current editor version doesn't correspond the one from the
        // project loaded
    }

    load_asset_files<assets::Tileset>(dir);
    load_asset_files<assets::Texture>(dir);
    load_asset_files<assets::Map>(dir);
}

} // namespace arpiyi_editor::serializer