#include "serializer.hpp"

#include "noc_file_dialog.h"

#include <fstream>

#include "map_manager.hpp"
#include "project_info.hpp"
#include "tileset_manager.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace arpiyi_editor;

namespace project_file_definitions {
constexpr std::string_view tilesets_path_json_key = "tilesets_path";
constexpr std::string_view default_tilesets_path = "tilesets";
constexpr std::string_view maps_path_json_key = "maps_path";
constexpr std::string_view default_maps_path = "maps";

constexpr std::string_view editor_version_json_key = "editor_version";
} // namespace project_file_definitions

static void save_project_file(fs::path base_dir) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace project_file_definitions;

    w.StartObject();
    {
        // Create tilesets directory
        w.Key(tilesets_path_json_key.data());
        fs::create_directory(base_dir / default_tilesets_path);
        w.String(default_tilesets_path.data());

        // Create maps directory
        w.Key(maps_path_json_key.data());
        fs::create_directory(base_dir / default_maps_path);
        w.String(default_maps_path.data());

        w.Key(editor_version_json_key.data());
        w.String(ARPIYI_EDITOR_VERSION);
    }
    w.EndObject();
    {
        std::ofstream project_file(base_dir / "project.json");
        project_file << s.GetString();
    }
}

namespace tileset_file_definitions {
constexpr std::string_view id_json_key = "id";
constexpr std::string_view name_json_key = "name";
constexpr std::string_view autotype_json_key = "auto_type";
constexpr std::string_view texture_path_json_key = "texture_path";
constexpr std::string_view textures_path = "textures";
} // namespace tileset_file_definitions

static void save_tileset_files(fs::path base_dir) {
    using namespace tileset_file_definitions;
    fs::path tileset_textures_path =
        base_dir / project_file_definitions::default_tilesets_path / textures_path;
    fs::create_directory(tileset_textures_path);
    for (const auto& tileset : tileset_manager::get_tilesets()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> w(s);

        w.StartObject();
        {
            w.Key(id_json_key.data());
            w.Uint64(tileset.get_id());

            w.Key(name_json_key.data());
            w.String(tileset.get()->name.data());

            w.Key(autotype_json_key.data());
            w.Uint64(static_cast<u64>(tileset.get()->auto_type));

            w.Key(texture_path_json_key.data());
            const std::string texture_filename = (tileset.get()->name + ".png");
            const fs::path tileset_texture_path =
                fs::relative(tileset_textures_path / texture_filename, base_dir);
            if (!fs::exists(tileset_texture_path)) {
                std::ofstream empty_file;
                empty_file.open(base_dir / tileset_texture_path, std::ios::out);
            }
            tileset.get()->texture.save({base_dir / tileset_texture_path});
            w.String(tileset_texture_path.c_str());
        }
        w.EndObject();
        {
            const std::string tileset_filename =
                std::to_string(tileset.get_id()) + "_" + tileset.get()->name + ".json";
            std::ofstream tileset_file(base_dir / project_file_definitions::default_tilesets_path /
                                       tileset_filename);
            tileset_file << s.GetString();
        }
    }
}

namespace map_file_definitions {
constexpr std::string_view id_json_key = "id";
constexpr std::string_view name_json_key = "name";
constexpr std::string_view width_json_key = "width";
constexpr std::string_view height_json_key = "height";
constexpr std::string_view layers_json_key = "layers";
namespace layer_file_definitions {
constexpr std::string_view name_json_key = "name";
constexpr std::string_view data_json_key = "data";
} // namespace layer_file_definitions
} // namespace map_file_definitions

static void save_map_files(fs::path base_dir) {
    using namespace map_file_definitions;

    for (const auto& map : map_manager::get_maps()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> w(s);

        w.StartObject();
        w.Key(id_json_key.data());
        w.Uint64(map.get_id());
        w.Key(width_json_key.data());
        w.Int64(map.get()->width);
        w.Key(height_json_key.data());
        w.Int64(map.get()->height);
        w.Key(layers_json_key.data());
        w.StartArray();
        for (const auto& layer : map.get()->layers) {
            namespace lfd = layer_file_definitions;
            w.StartObject();
            w.Key(lfd::name_json_key.data());
            w.String(layer.name.data());
            w.Key(lfd::data_json_key.data());
            w.StartArray();
            for (int x = 0; x < map.get()->width; ++x) {
                for (int y = 0; y < map.get()->height; ++y) { w.Uint(layer.get_tile({x, y}).id); }
            }
            w.EndArray();
            w.EndObject();
        }
        w.EndArray();
        w.EndObject();

        {
            const std::string map_filename =
                std::to_string(map.get_id()) + "_" + map.get()->name + ".json";
            std::ofstream map_file(base_dir / project_file_definitions::default_maps_path /
                                   map_filename);
            map_file << s.GetString();
        }
    }
}

namespace arpiyi_editor::serializer {

void save_project(fs::path dir) {
    save_project_file(dir);
    save_tileset_files(dir);
    save_map_files(dir);
}

void load_project(fs::path dir) { assert(false); }

} // namespace arpiyi_editor::serializer