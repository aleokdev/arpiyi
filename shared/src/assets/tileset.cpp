#include "assets/tileset.hpp"
#include "global_tile_size.hpp"

#include <algorithm>
#include <rapidjson/document.h>
#include <set>
#include <iostream>
#include <array>

#include "util/defs.hpp"
#include "util/intdef.hpp"

namespace arpiyi::assets {

math::IVec2D Tileset::get_size_in_tiles() const {
    auto tex = texture.get();
    return math::IVec2D{static_cast<i32>(tex->w / global_tile_size::get()),
                        static_cast<i32>(tex->h / global_tile_size::get())};
}

math::Rect2D Tileset::get_uv(u32 id) const {
    math::IVec2D size = get_size_in_tiles();
    std::uint32_t tile_pos_x = id % size.x;
    std::uint32_t tile_pos_y = id / size.x;
    aml::Vector2 start_uv_pos = {1.f / static_cast<float>(size.x) * static_cast<float>(tile_pos_x),
                                 1.f / static_cast<float>(size.y) * static_cast<float>(tile_pos_y)};
    aml::Vector2 end_uv_pos = {start_uv_pos.x + 1.f / static_cast<float>(size.x),
                               start_uv_pos.y + 1.f / static_cast<float>(size.y)};

    return {start_uv_pos, end_uv_pos};
}

// TODO: Document this
math::Rect2D Tileset::get_uv(u32 id, u8 minitile) const {
    // https://blog.rpgmakerweb.com/tutorials/anatomy-of-an-autotile/
    // Check if minitile given is valid
    assert(minitile >= 0 && minitile < 4);

    const u32 auto_tile_index = get_auto_tile_index_from_auto_id(id);
    const u8 surroundings = get_surroundings_from_auto_id(id);
    u8 vertical_flag;
    u8 horizontal_flag;
    u8 corner_flag;
    switch (minitile) {
        case (0): {
            // Upper-left minitile.
            // In bits:
            // c v -
            // h x -
            // - - -
            // Where c = corner, v = vertical & h = horizontal flag.
            vertical_flag = 0b00000010;
            horizontal_flag = 0b00001000;
            corner_flag = 0b00000001;
        } break;

        case (1): {
            // Upper-right minitile.
            // In bits:
            // - v c
            // - x h
            // - - -
            // Where c = corner, v = vertical & h = horizontal flag.
            vertical_flag = 0b00000010;
            horizontal_flag = 0b00010000;
            corner_flag = 0b00000100;
        } break;

        case (2): {
            // Lower-left minitile.
            // In bits:
            // - - -
            // h x -
            // c v -
            // Where c = corner, v = vertical & h = horizontal flag.
            vertical_flag = 0b01000000;
            horizontal_flag = 0b00001000;
            corner_flag = 0b00100000;
        } break;

        case (3): {
            // Lower-left minitile.
            // In bits:
            // - - -
            // - x h
            // - v c
            // Where c = corner, v = vertical & h = horizontal flag.
            vertical_flag = 0b01000000;
            horizontal_flag = 0b00010000;
            corner_flag = 0b10000000;
        } break;

        default: ARPIYI_UNREACHABLE(); throw;
    }

    constexpr math::IVec2D rpgmaker_a2_auto_tile_size_in_tiles{2, 3};
    const math::IVec2D size_of_tileset_in_tiles = get_size_in_tiles();
    const math::IVec2D size_of_tileset_in_auto_tiles{
        size_of_tileset_in_tiles.x / rpgmaker_a2_auto_tile_size_in_tiles.x,
        size_of_tileset_in_tiles.y / rpgmaker_a2_auto_tile_size_in_tiles.y};
    const aml::Vector2 tile_uv_size{1.f / static_cast<float>(size_of_tileset_in_tiles.x),
                                    1.f / static_cast<float>(size_of_tileset_in_tiles.y)};
    const aml::Vector2 auto_tile_start_uv_pos{
        static_cast<float>(auto_tile_index % size_of_tileset_in_auto_tiles.x) *
            rpgmaker_a2_auto_tile_size_in_tiles.x * tile_uv_size.x,
        static_cast<float>(auto_tile_index / size_of_tileset_in_auto_tiles.x) *
            rpgmaker_a2_auto_tile_size_in_tiles.y * tile_uv_size.y};

    /* clang-format off */
    /// Where each minitile is located locally in the RPGMaker A2 layout.
    const std::array<math::IVec2D, 20> rpgmaker_a2_layout = {
        /* A1 */ math::IVec2D{2, 0},
        /* A2 */ math::IVec2D{0, 2},
        /* A3 */ math::IVec2D{2, 4},
        /* A4 */ math::IVec2D{2, 2},
        /* A5 */ math::IVec2D{0, 4},
        /* B1 */ math::IVec2D{3, 0},
        /* B2 */ math::IVec2D{3, 2},
        /* B3 */ math::IVec2D{1, 4},
        /* B4 */ math::IVec2D{1, 2},
        /* B5 */ math::IVec2D{3, 4},
        /* C1 */ math::IVec2D{2, 1},
        /* C2 */ math::IVec2D{0, 5},
        /* C3 */ math::IVec2D{2, 3},
        /* C4 */ math::IVec2D{2, 5},
        /* C5 */ math::IVec2D{0, 3},
        /* D1 */ math::IVec2D{3, 1},
        /* D2 */ math::IVec2D{3, 5},
        /* D3 */ math::IVec2D{1, 3},
        /* D4 */ math::IVec2D{1, 5},
        /* D5 */ math::IVec2D{3, 3}
    };
    /* clang-format on */

    // A2 Autotile Format: https://imgur.com/a/vlRJ9cY
    u8 layout_index = minitile * 5;
    const bool connects_horizontally = !(horizontal_flag & surroundings);
    const bool connects_vertically = !(vertical_flag & surroundings);
    const bool connects_via_corner = !(corner_flag & surroundings);
    // Minitile/Corner-edge Relation: https://imgur.com/a/Fb58R2E
    switch ((connects_via_corner << 2u) | (connects_vertically << 1u) | (connects_horizontally)) {
        case (0b100):
        case (0b000): {
            layout_index += 1; // X2; No connections
        } break;
        case (0b110):
        case (0b010): {
            layout_index += 4; // X5; Horizontal connection
        } break;
        case (0b101):
        case (0b001): {
            layout_index += 3; // X4; Vertical connection
        } break;
        case (0b111): {
            layout_index += 2; // X3; All connected
        } break;
        case (0b011): {
            /* layout_index += 0; */ // X1; All connected except corner
        } break;
    }

    const aml::Vector2 uv_start =
        auto_tile_start_uv_pos +
        aml::Vector2{static_cast<float>(rpgmaker_a2_layout[layout_index].x) * tile_uv_size.x / 2.f,
                     static_cast<float>(rpgmaker_a2_layout[layout_index].y) * tile_uv_size.y / 2.f};
    const aml::Vector2 uv_end = uv_start + tile_uv_size / 2.f;
    return math::Rect2D{uv_start, uv_end};
}

u32 Tileset::get_id_autotype_none(math::IVec2D pos) const {
    auto tex = texture.get();
    return pos.x + pos.y * static_cast<u32>(tex->w / global_tile_size::get());
}

u32 Tileset::get_id_autotype_rpgmaker_a2(u32 auto_tile_index, u8 surroundings) const {
    return (auto_tile_index << 8u) + surroundings;
}

u8 Tileset::get_surroundings_from_auto_id(u32 id) const { return id & 0xFFu; }

u32 Tileset::get_auto_tile_index_from_auto_id(u32 id) const { return id >> 8u; }

u32 Tileset::get_auto_tile_count() const {
    switch (auto_type) {
        case (AutoType::rpgmaker_a2): {
            math::IVec2D size_in_tiles = get_size_in_tiles();
            // The RPGMaker A2 format uses a 2x3 tile area for storing each autotile.
            int x_auto_tiles = size_in_tiles.x / 2;
            int y_auto_tiles = size_in_tiles.y / 3;
            return x_auto_tiles * y_auto_tiles;
        }
        default:
            return 0;
    }
}

namespace tileset_file_definitions {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view autotype_json_key = "auto_type";
constexpr std::string_view texture_id_json_key = "texture_id";

} // namespace tileset_file_definitions

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace tileset_file_definitions;

    w.StartObject();
    {
        w.Key(name_json_key.data());
        w.String(tileset.name.data());

        w.Key(autotype_json_key.data());
        w.Uint64(static_cast<u64>(tileset.auto_type));

        w.Key(texture_id_json_key.data());
        w.Uint64(tileset.texture.get_id());
    }
    w.EndObject();
    {
        RawSaveData data;
        data.bytestream.write(s.GetString(), s.GetLength());
        return data;
    }
}

template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    using namespace tileset_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == name_json_key.data()) {
            tileset.name = obj.value.GetString();
        } else if (obj.name == autotype_json_key.data()) {
            u32 auto_type = obj.value.GetUint();

            // TODO: Move check to load_tilesets and throw a proper exception
            assert(auto_type >= static_cast<u32>(assets::Tileset::AutoType::none) &&
                   auto_type < static_cast<u32>(assets::Tileset::AutoType::count));

            tileset.auto_type = static_cast<assets::Tileset::AutoType>(auto_type);
        } else if (obj.name == texture_id_json_key.data()) {
            tileset.texture = Handle<assets::Texture>(obj.value.GetUint64());
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi::assets