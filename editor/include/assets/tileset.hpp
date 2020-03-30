#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "util/math.hpp"

#include <string>

namespace arpiyi_editor::assets {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    std::string name;

    /// Returns the size of this tileset in tiles, taking the tilesize from tileset_manager::get_tile_size().
    [[nodiscard]] math::IVec2D get_size_in_tiles() const;

    /// Returns the size of this tileset in tiles, taking the tilesize as an argument.
    [[nodiscard]] math::IVec2D get_size_in_tiles(std::size_t tile_size) const;

    [[nodiscard]] math::Rect2D get_uv(std::uint32_t id) const;

    [[nodiscard]] math::Rect2D get_uv(std::uint32_t id, std::size_t tile_size) const;

    [[nodiscard]] std::uint32_t get_id(math::IVec2D pos);
};

}

#endif // ARPIYI_TILESET_HPP
