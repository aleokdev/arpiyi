#include "assets/map.hpp"
#include "global_tile_size.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi::assets {

Mesh Map::Layer::generate_layer_split_quad() {
    // Format: {pos.x pos.y uv.x uv.y ...}
    // 2 because it's 2 position coords and 2 UV coords.
    constexpr auto sizeof_vertex = 4;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = height * width * sizeof_quad;

    std::vector<float> result(sizeof_splitted_quad);
    const float x_slice_size = 1.f / width;
    const float y_slice_size = 1.f / height;
    assert(tileset.get());
    const auto& tl = *tileset.get();
    // Create a quad for each {x, y} position.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const float min_vertex_x_pos = static_cast<float>(x) * x_slice_size;
            const float min_vertex_y_pos = static_cast<float>((int)height - y - 1) * y_slice_size;
            const float max_vertex_x_pos = min_vertex_x_pos + x_slice_size;
            const float max_vertex_y_pos = min_vertex_y_pos + y_slice_size;

            const math::Rect2D uv_pos = tl.get_uv(get_tile({x, y}).id);

            const auto quad_n = (x + y * width) * sizeof_quad;
            // First triangle //
            /* X pos 1st vertex */ result[quad_n + 0] = min_vertex_x_pos;
            /* Y pos 1st vertex */ result[quad_n + 1] = min_vertex_y_pos;
            /* X UV 1st vertex  */ result[quad_n + 2] = uv_pos.start.x;
            /* Y UV 1st vertex  */ result[quad_n + 3] = uv_pos.start.y;
            /* X pos 2nd vertex */ result[quad_n + 4] = max_vertex_x_pos;
            /* Y pos 2nd vertex */ result[quad_n + 5] = min_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 6] = uv_pos.end.x;
            /* Y UV 2nd vertex  */ result[quad_n + 7] = uv_pos.start.y;
            /* X pos 3rd vertex */ result[quad_n + 8] = min_vertex_x_pos;
            /* Y pos 3rd vertex */ result[quad_n + 9] = max_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 10] = uv_pos.start.x;
            /* Y UV 2nd vertex  */ result[quad_n + 11] = uv_pos.end.y;

            // Second triangle //
            /* X pos 1st vertex */ result[quad_n + 12] = max_vertex_x_pos;
            /* Y pos 1st vertex */ result[quad_n + 13] = min_vertex_y_pos;
            /* X UV 1st vertex  */ result[quad_n + 14] = uv_pos.end.x;
            /* Y UV 1st vertex  */ result[quad_n + 15] = uv_pos.start.y;
            /* X pos 2nd vertex */ result[quad_n + 16] = max_vertex_x_pos;
            /* Y pos 2nd vertex */ result[quad_n + 17] = max_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 18] = uv_pos.end.x;
            /* Y UV 2nd vertex  */ result[quad_n + 19] = uv_pos.end.y;
            /* X pos 3rd vertex */ result[quad_n + 20] = min_vertex_x_pos;
            /* Y pos 3rd vertex */ result[quad_n + 21] = max_vertex_y_pos;
            /* X UV 3rd vertex  */ result[quad_n + 22] = uv_pos.start.x;
            /* Y UV 3rd vertex  */ result[quad_n + 23] = uv_pos.end.y;
        }
    }

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_splitted_quad * sizeof(float), result.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(vao);
    // Vertex Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(0, 0);
    // UV Positions
    glEnableVertexAttribArray(1); // location 1
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glBindVertexBuffer(1, vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(1, 1);

    return Mesh{vao, vbo};
}

Map::Layer::Layer(i64 width, i64 height, Handle<assets::Tileset> t) :
    tileset(t), width(width), height(height), tiles(width * height) {
    regenerate_mesh();
}

void Map::Layer::regenerate_mesh() {
    if (tileset.get()) {
        mesh.unload();
        mesh = asset_manager::put(generate_layer_split_quad());
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
                        if(layer.name == "") {
                            layer.name = "<Blank name>";
                        }
                    } else if (layer_val.name == lfd::tileset_id_json_key.data()) {
                        layer.tileset = Handle<Tileset>(layer_val.value.GetUint64());
                        layer.regenerate_mesh();
                    } else if (layer_val.name == lfd::data_json_key.data()) {
                        u64 i = 0;
                        for (auto const& layer_tile : layer_val.value.GetArray()) {
                            layer.set_tile(
                                {static_cast<i32>(i % map.width), static_cast<i32>(i / map.width)},
                                {layer_tile.GetUint()});
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
} // namespace arpiyi_editor::assets
