#include "assets/map.hpp"
#include "global_tile_size.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "util/defs.hpp"

#include <vector>

namespace arpiyi::assets {

// TODO: Codegenize this
PiecedSprite Map::Tile::sprite(TileSurroundings const& surroundings) const {
    assert(parent.tileset.get());
    switch (parent.tileset.get()->tile_type) {
        case TileType::normal: return impl_sprite<TileType::normal>(surroundings);
            //case TileType::rpgmaker_a2: return impl_sprite<TileType::rpgmaker_a2>(surroundings);
        default: assert(false && "Unknown tileset type"); return {};
    }
}

Map::TileConnections Map::Tile::calculate_connections(TileSurroundings const& surroundings) const {
    if (override_connections)
        return custom_connections;
    /* clang-format off */
    return {
        surroundings.down.tileset == parent.tileset && surroundings.down.tile_index == parent.tile_index,
        surroundings.down_right.tileset == parent.tileset && surroundings.down_right.tile_index == parent.tile_index,
        surroundings.right.tileset == parent.tileset && surroundings.right.tile_index == parent.tile_index,
        surroundings.up_right.tileset == parent.tileset && surroundings.up_right.tile_index == parent.tile_index,
        surroundings.up.tileset == parent.tileset && surroundings.up.tile_index == parent.tile_index,
        surroundings.up_left.tileset == parent.tileset && surroundings.up_left.tile_index == parent.tile_index,
        surroundings.left.tileset == parent.tileset && surroundings.left.tile_index == parent.tile_index,
        surroundings.down_left.tileset == parent.tileset && surroundings.down_left.tile_index == parent.tile_index
    };
    /* clang-format on */
}

[[nodiscard]] Map::TileSurroundings Map::Layer::get_surroundings(math::IVec2D pos) {
    Map::TileSurroundings surroundings;
    const auto get_or_null = [this](math::IVec2D pos) -> Tileset::Tile* {
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

constexpr auto sizeof_vertex = 5;
constexpr auto sizeof_triangle = 3 * sizeof_vertex;
constexpr auto sizeof_quad = 2 * sizeof_triangle;

// TODO: Move OpenGL code elsewhere

void add_quad(std::vector<float>& result,
              int quad_n,
              math::Rect2D pos_rect,
              float min_z,
              float max_z,
              math::Rect2D uv_rect) {
    const auto tri_n = quad_n * sizeof_quad;
    if (tri_n >= result.size()) {
        // Allocate more size if needed
        result.resize(result.size() + 16 * sizeof_quad);
    }

    // First triangle //
    /* X pos 1st vertex */ result[tri_n + 0] = pos_rect.start.x;
    /* Y pos 1st vertex */ result[tri_n + 1] = pos_rect.start.y;
    /* Z pos 1st vertex */ result[tri_n + 2] = min_z;
    /* X UV 1st vertex  */ result[tri_n + 3] = uv_rect.start.x;
    /* Y UV 1st vertex  */ result[tri_n + 4] = uv_rect.end.y;
    /* X pos 2nd vertex */ result[tri_n + 5] = pos_rect.end.x;
    /* Y pos 2nd vertex */ result[tri_n + 6] = pos_rect.start.y;
    /* Z pos 2nd vertex */ result[tri_n + 7] = min_z;
    /* X UV 2nd vertex  */ result[tri_n + 8] = uv_rect.end.x;
    /* Y UV 2nd vertex  */ result[tri_n + 9] = uv_rect.end.y;
    /* X pos 3rd vertex */ result[tri_n + 10] = pos_rect.start.x;
    /* Y pos 3rd vertex */ result[tri_n + 11] = pos_rect.end.y;
    /* Z pos 3rd vertex */ result[tri_n + 12] = max_z;
    /* X UV 2nd vertex  */ result[tri_n + 13] = uv_rect.start.x;
    /* Y UV 2nd vertex  */ result[tri_n + 14] = uv_rect.start.y;

    // Second triangle //
    /* X pos 1st vertex */ result[tri_n + 15] = pos_rect.end.x;
    /* Y pos 1st vertex */ result[tri_n + 16] = pos_rect.start.y;
    /* Z pos 1st vertex */ result[tri_n + 17] = min_z;
    /* X UV 1st vertex  */ result[tri_n + 18] = uv_rect.end.x;
    /* Y UV 1st vertex  */ result[tri_n + 19] = uv_rect.end.y;
    /* X pos 2nd vertex */ result[tri_n + 20] = pos_rect.end.x;
    /* Y pos 2nd vertex */ result[tri_n + 21] = pos_rect.end.y;
    /* Z pos 2nd vertex */ result[tri_n + 22] = max_z;
    /* X UV 2nd vertex  */ result[tri_n + 23] = uv_rect.end.x;
    /* Y UV 2nd vertex  */ result[tri_n + 24] = uv_rect.start.y;
    /* X pos 3rd vertex */ result[tri_n + 25] = pos_rect.start.x;
    /* Y pos 3rd vertex */ result[tri_n + 26] = pos_rect.end.y;
    /* Z pos 3rd vertex */ result[tri_n + 27] = max_z;
    /* X UV 3rd vertex  */ result[tri_n + 28] = uv_rect.start.x;
    /* Y UV 3rd vertex  */ result[tri_n + 29] = uv_rect.start.y;
};

void add_vertical_quad(
    std::vector<float>& result, int quad_n, math::Rect2D pos_rect, float min_z, float max_z) {
    const auto tri_n = quad_n * sizeof_quad;
    if (tri_n >= result.size()) {
        // Allocate more size if needed
        result.resize(result.size() + 16 * sizeof_quad);
    }

    // We use 1 for the UVs because in order for the depth shader to detect the wall, they must
    // have a non-transparent texture. All textures are set to clamp to a non-transparent
    // border, so we use that.

    // First triangle //
    /* X pos 1st vertex */ result[tri_n + 0] = pos_rect.start.x;
    /* Y pos 1st vertex */ result[tri_n + 1] = pos_rect.end.y;
    /* Z pos 1st vertex */ result[tri_n + 2] = min_z;
    /* X UV 1st vertex  */ result[tri_n + 3] = 1;
    /* Y UV 1st vertex  */ result[tri_n + 4] = 1;
    /* X pos 2nd vertex */ result[tri_n + 5] = pos_rect.end.x;
    /* Y pos 2nd vertex */ result[tri_n + 6] = pos_rect.start.y;
    /* Z pos 2nd vertex */ result[tri_n + 7] = max_z;
    /* X UV 2nd vertex  */ result[tri_n + 8] = 1;
    /* Y UV 2nd vertex  */ result[tri_n + 9] = 1;
    /* X pos 3rd vertex */ result[tri_n + 10] = pos_rect.end.x;
    /* Y pos 3rd vertex */ result[tri_n + 11] = pos_rect.start.y;
    /* Z pos 3rd vertex */ result[tri_n + 12] = min_z;
    /* X UV 2nd vertex  */ result[tri_n + 13] = 1;
    /* Y UV 2nd vertex  */ result[tri_n + 14] = 1;

    // Second triangle //
    /* X pos 1st vertex */ result[tri_n + 15] = pos_rect.start.x;
    /* Y pos 1st vertex */ result[tri_n + 16] = pos_rect.end.y;
    /* Z pos 1st vertex */ result[tri_n + 17] = min_z;
    /* X UV 1st vertex  */ result[tri_n + 18] = 1;
    /* Y UV 1st vertex  */ result[tri_n + 19] = 1;
    /* X pos 2nd vertex */ result[tri_n + 20] = pos_rect.start.x;
    /* Y pos 2nd vertex */ result[tri_n + 21] = pos_rect.end.y;
    /* Z pos 2nd vertex */ result[tri_n + 22] = max_z;
    /* X UV 2nd vertex  */ result[tri_n + 23] = 1;
    /* Y UV 2nd vertex  */ result[tri_n + 24] = 1;
    /* X pos 3rd vertex */ result[tri_n + 25] = pos_rect.end.x;
    /* Y pos 3rd vertex */ result[tri_n + 26] = pos_rect.start.y;
    /* Z pos 3rd vertex */ result[tri_n + 27] = max_z;
    /* X UV 3rd vertex  */ result[tri_n + 28] = 1;
    /* Y UV 3rd vertex  */ result[tri_n + 29] = 1;
};

Mesh Map::Layer::generate_mesh() {
    MeshBuilder builder;

    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            const auto tile = get_tile({x,y});
            // TODO: Implement slopes (Set y_min_z and y_max_z accordingly)
            builder.add_pieced_sprite(tile.sprite(get_surroundings({x, y})), {x, y, tile.height}, 0, 0);
        }
    }

    constexpr int quad_vertices = 2 * 3;
    const auto sizeof_result = quad_n * quad_vertices;

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_result * sizeof_vertex * sizeof(float), result.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(vao);
    // Vertex Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, vbo, 0, sizeof_vertex * sizeof(float));
    glVertexAttribBinding(0, 0);
    // UV Positions
    glEnableVertexAttribArray(1); // location 1
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glBindVertexBuffer(1, vbo, 0, sizeof_vertex * sizeof(float));
    glVertexAttribBinding(1, 1);

    return Mesh{vao, vbo, sizeof_result};
}

Mesh Map::Layer::generate_layer_mesh() {
    assert(tileset.get());
    switch (tileset.get()->auto_type) {
        case Tileset::AutoType::none: return generate_mesh<false>();
        case Tileset::AutoType::rpgmaker_a2: return generate_mesh<true>();
        default: ARPIYI_UNREACHABLE();
    }
}

Map::Layer::Layer(i64 width, i64 height, Handle<assets::Tileset> t) :
    tileset(t), width(width), height(height), tiles(width * height) {
    regenerate_mesh();
}

void Map::Layer::regenerate_mesh() {
    if (tileset.get()) {
        mesh.unload();
        mesh = asset_manager::put(generate_layer_mesh());
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
constexpr std::string_view depth_data_json_key = "depth";
constexpr std::string_view tileset_id_json_key = "tileset";
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
        w.Key(lfd::tileset_id_json_key.data());
        w.Uint64(layer.tileset.get_id());
        w.Key(lfd::data_json_key.data());
        w.StartArray();
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) { w.Uint(layer.get_tile({x, y}).id); }
        }
        w.EndArray();
        w.Key(lfd::depth_data_json_key.data());
        w.StartArray();
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) { w.Int(layer.get_tile({x, y}).height); }
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
                auto& layer = *map.layers
                                   .emplace_back(asset_manager::put(
                                       assets::Map::Layer(map.width, map.height, -1)))
                                   .get();

                for (auto const& layer_val : layer_object.GetObject()) {
                    if (layer_val.name == lfd::name_json_key.data()) {
                        layer.name = layer_val.value.GetString();
                        // < 0.1.4 compatibility: Blank layer names are no longer allowed
                        if (layer.name == "") {
                            layer.name = "<Blank name>";
                        }
                    } else if (layer_val.name == lfd::tileset_id_json_key.data()) {
                        layer.tileset = Handle<Tileset>(layer_val.value.GetUint64());
                        layer.regenerate_mesh();
                    } else if (layer_val.name == lfd::data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& layer_tile : layer_val.value.GetArray()) {
                            layer
                                .get_tile({static_cast<i32>(i % map.width),
                                           static_cast<i32>(i / map.width)})
                                .id = layer_tile.GetUint();
                            ++i;
                        }
                        layer.regenerate_mesh();
                    } else if (layer_val.name == lfd::depth_data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& tile_depth : layer_val.value.GetArray()) {
                            layer
                                .get_tile({static_cast<i32>(i % map.width),
                                           static_cast<i32>(i / map.width)})
                                .height = tile_depth.GetInt();
                            ++i;
                        }
                        layer.regenerate_mesh();
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
