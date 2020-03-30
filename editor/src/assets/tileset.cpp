#include "assets/tileset.hpp"
#include "tileset_manager.hpp"

namespace arpiyi_editor::assets {

math::IVec2D Tileset::get_size_in_tiles() const {
    const auto tile_size = tileset_manager::get_tile_size();
    auto tex = texture.const_get();
    return math::IVec2D{(int)tex->w/(int)tile_size, (int)tex->h/(int)tile_size};
}

math::IVec2D Tileset::get_size_in_tiles(std::size_t tile_size) const {
    auto tex = texture.const_get();
    return math::IVec2D{(int)tex->w/(int)tile_size, (int)tex->h/(int)tile_size};
}

math::Rect2D Tileset::get_uv(std::uint32_t id) const {
    return get_uv(id, tileset_manager::get_tile_size());
}

math::Rect2D Tileset::get_uv(std::uint32_t id, std::size_t tile_size) const {
    math::IVec2D size = get_size_in_tiles(tile_size);
    std::uint32_t tile_pos_x = id % size.x;
    std::uint32_t tile_pos_y = id / size.x;
    math::Vec2D start_uv_pos = {1.f / static_cast<float>(size.x) * static_cast<float>(tile_pos_x),
                                1.f / static_cast<float>(size.y) * static_cast<float>(tile_pos_y+1)};
    math::Vec2D end_uv_pos = {start_uv_pos.x + 1.f / static_cast<float>(size.x),
                              start_uv_pos.y - 1.f / static_cast<float>(size.y)};

    return {start_uv_pos, end_uv_pos};
}

std::uint32_t Tileset::get_id(math::IVec2D pos) {
    auto tex = texture.const_get();
    return pos.x + pos.y * (int)tex->w/(int)tileset_manager::get_tile_size();
}

}