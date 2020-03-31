#include "assets/tileset.hpp"
#include "tileset_manager.hpp"

namespace arpiyi_editor::assets {

math::IVec2D Tileset::get_size_in_tiles() const {
    const auto tile_size = tileset_manager::get_tile_size();
    auto tex = texture.get();
    return math::IVec2D{static_cast<i32>(tex->w/tile_size), static_cast<i32>(tex->h/tile_size)};
}

math::IVec2D Tileset::get_size_in_tiles(u32 tile_size) const {
    auto tex = texture.get();
    return math::IVec2D{static_cast<i32>(tex->w/tile_size), static_cast<i32>(tex->h/tile_size)};
}

math::Rect2D Tileset::get_uv(u32 id) const {
    return get_uv(id, tileset_manager::get_tile_size());
}

math::Rect2D Tileset::get_uv(u32 id, u32 tile_size) const {
    math::IVec2D size = get_size_in_tiles(tile_size);
    std::uint32_t tile_pos_x = id % size.x;
    std::uint32_t tile_pos_y = id / size.x;
    math::Vec2D start_uv_pos = {1.f / static_cast<float>(size.x) * static_cast<float>(tile_pos_x),
                                1.f / static_cast<float>(size.y) * static_cast<float>(tile_pos_y+1)};
    math::Vec2D end_uv_pos = {start_uv_pos.x + 1.f / static_cast<float>(size.x),
                              start_uv_pos.y - 1.f / static_cast<float>(size.y)};

    return {start_uv_pos, end_uv_pos};
}

u32 Tileset::get_id(math::IVec2D pos) const {
    auto tex = texture.get();
    return pos.x + pos.y * static_cast<u32>(tex->w/tileset_manager::get_tile_size());
}

}