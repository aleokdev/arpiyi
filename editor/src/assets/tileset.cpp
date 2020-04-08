#include "assets/tileset.hpp"
#include "tileset_manager.hpp"

#include <set>
#include <algorithm>
#include <rapidjson/document.h>

#include "util/intdef.hpp"

namespace arpiyi_editor::assets {

math::IVec2D Tileset::get_size_in_tiles() const {
    const auto tile_size = tileset_manager::get_tile_size();
    auto tex = texture.get();
    return math::IVec2D{static_cast<i32>(tex->w / tile_size), static_cast<i32>(tex->h / tile_size)};
}

math::IVec2D Tileset::get_size_in_tiles(u32 tile_size) const {
    auto tex = texture.get();
    return math::IVec2D{static_cast<i32>(tex->w / tile_size), static_cast<i32>(tex->h / tile_size)};
}

math::Rect2D Tileset::get_uv(u32 id) const { return get_uv(id, tileset_manager::get_tile_size()); }

math::Rect2D Tileset::get_uv(u32 id, u32 tile_size) const {
    math::IVec2D size = get_size_in_tiles(tile_size);
    std::uint32_t tile_pos_x = id % size.x;
    std::uint32_t tile_pos_y = id / size.x;
    math::Vec2D start_uv_pos = {1.f / static_cast<float>(size.x) * static_cast<float>(tile_pos_x),
                                1.f / static_cast<float>(size.y) *
                                    static_cast<float>(tile_pos_y + 1)};
    math::Vec2D end_uv_pos = {start_uv_pos.x + 1.f / static_cast<float>(size.x),
                              start_uv_pos.y - 1.f / static_cast<float>(size.y)};

    return {start_uv_pos, end_uv_pos};
}

u32 Tileset::get_id(math::IVec2D pos) const {
    auto tex = texture.get();
    return pos.x + pos.y * static_cast<u32>(tex->w / tileset_manager::get_tile_size());
}

static const std::set<u8> tile_table = {
    0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00000101,

    0b00001000, 0b00001010, 0b00001100,

    0b00010000, 0b00010001, 0b00010010, 0b00011000, 0b00011010,

    0b00010000, 0b00010001, 0b00010010, 0b00011000, 0b00011010,

    0b00100000, 0b00100001, 0b00100010, 0b00100100, 0b00100101, 0b00110000, 0b00110001, 0b00110010,

    0b01000000, 0b01000001, 0b01000010, 0b01000100, 0b01000101, 0b01001000, 0b01001010, 0b01001100,
    0b01010000, 0b01010001, 0b01010010, 0b01011000, 0b01011010,

    0b10000000, 0b10000001, 0b10000010, 0b10000100, 0b10000101, 0b10001000, 0b10001010, 0b10001100,
    0b10100000, 0b10100001, 0b10100010, 0b10100100, 0b10100101};

u32 Tileset::get_id_auto(u32 x_index, u32 surroundings) const {
    enum tile_sides {
        upper_left_corner = 1 << 0,
        upper_middle_side = 1 << 1,
        upper_right_corner = 1 << 2,
        middle_left_side = 1 << 3,
        middle_right_side = 1 << 4,
        lower_left_corner = 1 << 5,
        lower_middle_side = 1 << 6,
        lower_right_corner = 1 << 7
    };
    // Delete corners if there's a side next to them (since those cases are not covered by the tile
    // table)
    if (surroundings & upper_middle_side)
        surroundings &= (0xFF ^ upper_left_corner ^ upper_right_corner);
    if (surroundings & middle_left_side)
        surroundings &= (0xFF ^ upper_left_corner ^ lower_left_corner);
    if (surroundings & middle_right_side)
        surroundings &= (0xFF ^ upper_right_corner ^ lower_right_corner);
    if (surroundings & lower_middle_side)
        surroundings &= (0xFF ^ lower_left_corner ^ lower_right_corner);

    const auto it = tile_table.find(surroundings);
    if (it == tile_table.end())
        assert(false);
    return x_index + get_size_in_tiles(tileset_manager::get_tile_size()).x *
                         static_cast<u8>(std::distance(tile_table.begin(), it));
}

u32 Tileset::get_surroundings_from_auto_id(u32 id) const {
    const auto tileset_size_in_tiles{get_size_in_tiles()};
    const u32 auto_tiletable_index =
        (id - (id % tileset_size_in_tiles.x)) / tileset_size_in_tiles.x;
    return *std::next(tile_table.begin(), auto_tiletable_index);
}
u32 Tileset::get_x_index_from_auto_id(u32 id) const {
    const auto tileset_size_in_tiles{get_size_in_tiles()};
    return (id % tileset_size_in_tiles.x);
}

namespace tileset_file_definitions {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view autotype_json_key = "auto_type";
constexpr std::string_view texture_id_json_key = "texture_id";

} // namespace tileset_file_definitions

template<> void raw_save<Tileset>(Tileset const& tileset, SaveParams<Tileset> const& params) {
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
        std::ofstream tileset_file(params.save_path);
        tileset_file << s.GetString();
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
            tileset.texture = obj.value.GetUint64();
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi_editor::assets