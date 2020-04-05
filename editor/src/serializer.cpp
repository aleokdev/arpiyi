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
            w.String(tileset_texture_path.generic_string().c_str());
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
constexpr std::string_view tileset_id_json_key = "tileset";
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
        w.Key(name_json_key.data());
        w.String(map.get()->name.c_str());
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
            w.Key(lfd::tileset_id_json_key.data());
            w.Uint64(layer.tileset.get_id());
            w.Key(lfd::data_json_key.data());
            w.StartArray();
            for (int y = 0; y < map.get()->height; ++y) {
                for (int x = 0; x < map.get()->width; ++x) { w.Uint(layer.get_tile({x, y}).id); }
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

struct TilesetFileData {
    u64 id;
    std::string name;
    assets::Tileset::AutoType auto_type;
    fs::path texture_path;
};

static TilesetFileData load_tileset(fs::path tileset_path) {
    std::ifstream f(tileset_path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    TilesetFileData file_data;

    using namespace tileset_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == id_json_key.data()) {
            file_data.id = obj.value.GetUint64();
        } else if (obj.name == name_json_key.data()) {
            file_data.name = obj.value.GetString();
        } else if (obj.name == autotype_json_key.data()) {
            u32 auto_type = obj.value.GetUint();

            // TODO: Move check to load_tilesets and throw a proper exception
            assert(auto_type >= static_cast<u32>(assets::Tileset::AutoType::none) &&
                   auto_type < static_cast<u32>(assets::Tileset::AutoType::count));

            file_data.auto_type = static_cast<assets::Tileset::AutoType>(auto_type);
        } else if (obj.name == texture_path_json_key.data()) {
            file_data.texture_path = obj.value.GetString();
        } else
            assert("Unknown JSON key in tileset file");
    }

    return file_data;
}

static void load_tilesets(fs::path project_dir, fs::path tilesets_dir) {
    for (const auto& entry : fs::directory_iterator(tilesets_dir)) {
        if (!entry.is_regular_file())
            continue;

        TilesetFileData tile_data = load_tileset(entry.path());
        Handle<assets::Texture> tex =
            asset_manager::load<assets::Texture>({project_dir / tile_data.texture_path, false});

        asset_manager::put(assets::Tileset{tile_data.auto_type, tex, tile_data.name}, tile_data.id);
    }
}

struct MapFileData {
    u64 id;
    std::string name;
    struct LayerFileData {
        std::string name;
        u64 tileset_id;
        std::vector<u32> raw_tile_data;
    };
    std::vector<LayerFileData> layers;
    i64 width = -1, height = -1;
};

static MapFileData load_map(fs::path map_path) {
    std::ifstream f(map_path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    MapFileData file_data;

    using namespace map_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == id_json_key.data()) {
            file_data.id = obj.value.GetUint64();
        } else if (obj.name == name_json_key.data()) {
            file_data.name = obj.value.GetString();
        } else if (obj.name == width_json_key.data()) {
            file_data.width = obj.value.GetInt64();
        } else if (obj.name == height_json_key.data()) {
            file_data.height = obj.value.GetInt64();
        } else if (obj.name == layers_json_key.data()) {
            for (auto const& layer_object : obj.value.GetArray()) {
                namespace lfd = layer_file_definitions;
                auto& layer = file_data.layers.emplace_back();

                for (auto const& layer_val : layer_object.GetObject()) {
                    if (layer_val.name == lfd::name_json_key.data()) {
                        layer.name = layer_val.value.GetString();
                    } else if (layer_val.name == lfd::tileset_id_json_key.data()) {
                        layer.tileset_id = layer_val.value.GetUint64();
                    } else if (layer_val.name == lfd::data_json_key.data()) {
                        if (file_data.width != -1 && file_data.height != -1) {
                            layer.raw_tile_data.resize(file_data.width * file_data.height);
                        } else
                            assert("Layer data defined before width & height");

                        u64 i = 0;
                        for (auto const& layer_tile : layer_val.value.GetArray()) {
                            layer.raw_tile_data[i] = layer_tile.GetUint();
                            ++i;
                        }
                    }
                }
            }
        }
    }

    return file_data;
}

static void load_maps(fs::path maps_dir) {
    for (const auto& entry : fs::directory_iterator(maps_dir)) {
        if (!entry.is_regular_file())
            continue;

        MapFileData map_data = load_map(entry.path());
        assets::Map map;
        map.width = map_data.width;
        map.height = map_data.height;
        map.name = map_data.name;

        for (MapFileData::LayerFileData const& raw_layer : map_data.layers) {
            assets::Map::Layer& layer = map.layers.emplace_back(
                map.width, map.height, Handle<assets::Tileset>(raw_layer.tileset_id));
            layer.name = raw_layer.name;

            i32 i = 0;
            for (const auto& raw_tile : raw_layer.raw_tile_data) {
                layer.set_tile({static_cast<i32>(i % map.width), static_cast<i32>(i / map.width)},
                               {raw_tile});
                ++i;
            }
        }

        asset_manager::put(map, map_data.id);
    }
}

struct ProjectFileData {
    fs::path tilesets_path;
    fs::path maps_path;
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
        if (obj.name == tilesets_path_json_key.data()) {
            file_data.tilesets_path = obj.value.GetString();
        } else if (obj.name == maps_path_json_key.data()) {
            file_data.maps_path = obj.value.GetString();
        } else if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        }
    }

    return file_data;
}

namespace arpiyi_editor::serializer {

void save_project(fs::path dir) {
    save_project_file(dir);
    save_tileset_files(dir);
    save_map_files(dir);
}

void load_project(fs::path dir) {
    ProjectFileData file_data(load_project_file(dir));

    if (ARPIYI_EDITOR_VERSION != file_data.editor_version) {
        // TODO: do something if current editor version doesn't correspond the one from the
        // project loaded
    }

    if (file_data.tilesets_path.empty()) {
        assert("Tileset path is empty"); // TODO: Throw proper exception
    }

    if (file_data.maps_path.empty()) {
        assert("Maps path is empty"); // TODO: Throw proper exception
    }

    if (file_data.tilesets_path.is_relative()) {
        file_data.tilesets_path = dir / file_data.tilesets_path;
    }
    if (file_data.maps_path.is_relative()) {
        file_data.maps_path = dir / file_data.maps_path;
    }

    load_tilesets(dir, file_data.tilesets_path);
    load_maps(file_data.maps_path);
}

} // namespace arpiyi_editor::serializer