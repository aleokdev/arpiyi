#include "assets/map.hpp"
#include "global_tile_size.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "renderer/renderer.hpp"
#include "util/defs.hpp"

#include <vector>

namespace arpiyi::assets {

// TODO: Codegenize this
Sprite Map::Tile::sprite(Layer const& this_layer, math::IVec2D this_pos) const {
    if (!exists) {
        return Sprite{{Handle<assets::TextureAsset>::noid}};
    }
    assert(parent.tileset.get());
    switch (parent.tileset.get()->tile_type) {
        case TileType::normal: return impl_sprite<TileType::normal>(this_layer, this_pos);
        case TileType::rpgmaker_a2: return impl_sprite<TileType::rpgmaker_a2>(this_layer, this_pos);
        default: assert(false && "Unknown tileset type"); return {};
    }
}

Map::TileConnections Map::Tile::calculate_connections(TileSurroundings const& surroundings) const {
    if (override_connections)
        return custom_connections;
    const auto compare_or_false = [this](const assets::Tileset::Tile* tile) -> bool {
        if (tile == nullptr)
            return false;
        return tile->tileset == parent.tileset && tile->tile_index == parent.tile_index;
    };
    /* clang-format off */
    return {
        compare_or_false(surroundings.down),
        compare_or_false(surroundings.down_right),
        compare_or_false(surroundings.right),
        compare_or_false(surroundings.up_right),
        compare_or_false(surroundings.up),
        compare_or_false(surroundings.up_left),
        compare_or_false(surroundings.left),
        compare_or_false(surroundings.down_left)
    };
    /* clang-format on */
}

[[nodiscard]] Map::TileSurroundings Map::Layer::get_surroundings(math::IVec2D pos) const {
    Map::TileSurroundings surroundings;
    const auto get_or_null = [this](math::IVec2D pos) -> Tileset::Tile const* {
        return is_pos_valid(pos) ? &get_tile(pos).parent : nullptr;
    };

    surroundings.down = get_or_null({pos.x, pos.y - 1});
    surroundings.down_right = get_or_null({pos.x + 1, pos.y - 1});
    surroundings.right = get_or_null({pos.x + 1, pos.y});
    surroundings.up_right = get_or_null({pos.x + 1, pos.y + 1});
    surroundings.up = get_or_null({pos.x, pos.y + 1});
    surroundings.up_left = get_or_null({pos.x - 1, pos.y + 1});
    surroundings.left = get_or_null({pos.x - 1, pos.y});
    surroundings.down_left = get_or_null({pos.x - 1, pos.y - 1});

    return surroundings;
}

Map::Layer::Layer(i64 width, i64 height) : width(width), height(height), tiles(width * height) {}

void Map::draw_to_cmd_list(renderer::Renderer const& renderer,
                           renderer::DrawCmdList& cmd_list) {
    // TODO: Cache, cache, cache!!!!!!!!!!!
    std::unordered_map<u64, renderer::MeshBuilder> mesh_batches;
    static std::vector<renderer::MeshHandle> meshes;
    for (auto& mesh : meshes) { mesh.unload(); }
    meshes.clear();
    for (const auto& l : layers) {
        assert(l.get());
        const auto& layer = *l.get();
        if(!layer.visible) continue;

        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                const auto& tile = layer.get_tile({x, y});
                if (!tile.exists)
                    continue;
                const u64 tex_id = tile.parent.tileset.get()->texture.get_id();
                if (mesh_batches.find(tex_id) == mesh_batches.end())
                    mesh_batches[tex_id] = renderer::MeshBuilder();
                // TODO: Implement slopes
                mesh_batches[tex_id].add_sprite(
                    tile.sprite(layer, {x, y}),
                    {static_cast<float>(x), static_cast<float>(y), static_cast<float>(tile.height)},
                    0, 0);
            }
        }
    }
    for (auto& [tex_id, builder] : mesh_batches) {
        cmd_list.commands.emplace_back(
            renderer::DrawCmd{Handle<assets::TextureAsset>(tex_id).get()->handle,
                              meshes.emplace_back(builder.finish()),
                              renderer.lit_shader(),
                              {{0, 0, 0}},
                              true});
    }
    for (const auto& entity_handle : entities) {
        assert(entity_handle.get());
        if (auto entity = entity_handle.get()) {
            if (auto sprite = entity->sprite.get()) {
                assert(sprite->texture.get());

                // TODO: Cache sprite meshes
                renderer::MeshBuilder builder;
                builder.add_sprite(*sprite, {0,0,0}, 0, 0);

                cmd_list.commands.emplace_back(
                    renderer::DrawCmd{Handle<assets::TextureAsset>(sprite->texture).get()->handle,
                                      meshes.emplace_back(builder.finish()),
                                      renderer.lit_shader(),
                                      {aml::Vector3(entity->pos, 0.1f)},
                                      true});
            }
        }
    }
}

namespace map_file_definitions {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view width_json_key = "width";
constexpr std::string_view height_json_key = "height";
constexpr std::string_view layers_json_key = "layers";
constexpr std::string_view comments_json_key = "comments";
constexpr std::string_view entities_json_key = "entities";

namespace layer_file_definitions {
constexpr std::string_view name_json_key = "name";
constexpr std::string_view data_json_key = "data";
} // namespace layer_file_definitions

namespace comment_file_definitions {
constexpr std::string_view position_json_key = "pos";
constexpr std::string_view text_json_key = "text";
} // namespace comment_file_definitions

} // namespace map_file_definitions

template<> RawSaveData raw_get_save_data<Map>(Map const& map) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace map_file_definitions;

    w.StartObject();
    w.Key(name_json_key.data());
    w.String(map.name.c_str());
    w.Key(width_json_key.data());
    w.Int64(map.width);
    w.Key(height_json_key.data());
    w.Int64(map.height);
    w.Key(layers_json_key.data());
    w.StartArray();
    for (const auto& _l : map.layers) {
        auto& layer = *_l.get();
        namespace lfd = layer_file_definitions;
        w.StartObject();
        w.Key(lfd::name_json_key.data());
        w.String(layer.name.data());
        w.Key(lfd::data_json_key.data());
        w.StartArray();
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                const auto& tile = layer.get_tile({x, y});
                w.StartObject();
                if (!tile.exists) {
                    w.Key("exists");
                    w.Bool(false);
                } else {
                    {
                        w.Key("parent");
                        w.StartObject();
                        w.Key("tileset");
                        w.Uint64(tile.parent.tileset.get_id());
                        w.Key("tile_index");
                        w.Uint64(tile.parent.tile_index);
                        w.EndObject();
                    }
                    if (tile.override_connections) {
                        w.Key("custom_connections");
                        w.StartArray();
                        w.Bool(tile.custom_connections.down);
                        w.Bool(tile.custom_connections.down_right);
                        w.Bool(tile.custom_connections.right);
                        w.Bool(tile.custom_connections.up_right);
                        w.Bool(tile.custom_connections.up);
                        w.Bool(tile.custom_connections.up_left);
                        w.Bool(tile.custom_connections.left);
                        w.Bool(tile.custom_connections.down_left);
                        w.EndArray();
                    }
                    {
                        w.Key("height");
                        w.Int(tile.height);
                    }
                    {
                        w.Key("slope_type");
                        w.Uint(static_cast<u8>(tile.slope_type));
                    }
                    {
                        w.Key("has_side_walls");
                        w.Bool(tile.has_side_walls);
                    }
                }
                w.EndObject();
            }
        }
        w.EndArray();
        w.EndObject();
    }
    w.EndArray();
    w.Key(comments_json_key.data());
    w.StartArray();
    for (const auto& c : map.comments) {
        namespace cfd = comment_file_definitions;
        assert(c.get());
        const auto& comment = *c.get();
        w.StartObject();
        w.Key(cfd::text_json_key.data());
        w.String(comment.text.c_str());
        w.Key(cfd::position_json_key.data());
        w.StartObject();
        {
            w.Key("x");
            w.Int(comment.pos.x);
            w.Key("y");
            w.Int(comment.pos.y);
        }
        w.EndObject();
        w.EndObject();
    }
    w.EndArray();

    w.Key(entities_json_key.data());
    w.StartArray();
    for (const auto& c : map.entities) { w.Uint64(c.get_id()); }
    w.EndArray();

    w.EndObject();

    RawSaveData data;
    data.bytestream.write(s.GetString(), s.GetLength());

    return data;
}

template<> void raw_load<Map>(Map& map, LoadParams<Map> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    map.width = -1;
    map.height = -1;

    using namespace map_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == name_json_key.data()) {
            map.name = obj.value.GetString();
        } else if (obj.name == width_json_key.data()) {
            map.width = obj.value.GetInt64();
        } else if (obj.name == height_json_key.data()) {
            map.height = obj.value.GetInt64();
        } else if (obj.name == layers_json_key.data()) {
            for (auto const& layer_object : obj.value.GetArray()) {
                namespace lfd = layer_file_definitions;
                if (map.width == -1 || map.height == -1) {
                    assert("Map layer data loaded before width/height");
                }
                auto& layer =
                    *map.layers.emplace_back(asset_manager::put(Map::Layer(map.width, map.height)))
                         .get();

                for (auto const& layer_val : layer_object.GetObject()) {
                    if (layer_val.name == lfd::name_json_key.data()) {
                        layer.name = layer_val.value.GetString();
                        // < 0.1.4 compatibility: Blank layer names are no longer allowed
                        if (layer.name == "") {
                            layer.name = "<Blank name>";
                        }
                    } else if (layer_val.name == lfd::data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& tile_object_obj : layer_val.value.GetArray()) {
                            auto const& tile_object = tile_object_obj.GetObject();
                            auto& layer_tile = layer.get_tile(
                                {static_cast<i32>(i % map.width), static_cast<i32>(i / map.width)});
                            if (tile_object.HasMember("exists"))
                                layer_tile.exists = tile_object["exists"].GetBool();
                            else {
                                layer_tile.exists = true;
                                layer_tile.parent.tileset =
                                    Handle<Tileset>(tile_object["parent"]["tileset"].GetUint64());
                                layer_tile.parent.tile_index =
                                    tile_object["parent"]["tile_index"].GetUint64();
                                if (tile_object.HasMember("custom_connections")) {
                                    layer_tile.override_connections = true;
                                    const auto& custom_connections_arr =
                                        tile_object["custom_connections"].GetArray();
                                    layer_tile.custom_connections.down =
                                        custom_connections_arr[0].GetBool();
                                    layer_tile.custom_connections.down_right =
                                        custom_connections_arr[1].GetBool();
                                    layer_tile.custom_connections.right =
                                        custom_connections_arr[2].GetBool();
                                    layer_tile.custom_connections.up_right =
                                        custom_connections_arr[3].GetBool();
                                    layer_tile.custom_connections.up =
                                        custom_connections_arr[4].GetBool();
                                    layer_tile.custom_connections.up_left =
                                        custom_connections_arr[5].GetBool();
                                    layer_tile.custom_connections.left =
                                        custom_connections_arr[6].GetBool();
                                    layer_tile.custom_connections.down_left =
                                        custom_connections_arr[7].GetBool();
                                } else
                                    layer_tile.override_connections = false;
                                layer_tile.height = tile_object["height"].GetInt();
                                layer_tile.slope_type = static_cast<Map::Tile::SlopeType>(
                                    tile_object["slope_type"].GetUint());
                                layer_tile.has_side_walls = tile_object["has_side_walls"].GetBool();
                            }
                            ++i;
                        }
                    }
                }
            }
        } else if (obj.name == comments_json_key.data()) {
            for (auto const& comment_object : obj.value.GetArray()) {
                namespace cfd = comment_file_definitions;

                assets::Map::Comment comment;

                for (auto const& comment_val : comment_object.GetObject()) {
                    if (comment_val.name == cfd::text_json_key.data()) {
                        comment.text = comment_val.value.GetString();
                    } else if (comment_val.name == cfd::position_json_key.data()) {
                        comment.pos.x = comment_val.value.GetObject()["x"].GetInt();
                        comment.pos.y = comment_val.value.GetObject()["y"].GetInt();
                    }
                }

                map.comments.emplace_back(asset_manager::put(comment));
            }
        } else if (obj.name == entities_json_key.data()) {
            for (const auto& entity_id : obj.value.GetArray()) {
                map.entities.emplace_back(entity_id.GetUint64());
            }
        }
    }
}
} // namespace arpiyi::assets
