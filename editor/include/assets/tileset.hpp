#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "util/math.hpp"
#include "util/intdef.hpp"

#include <string>

namespace arpiyi_editor::assets {

struct Tileset {
    enum class AutoType {
        none,
        // Used for RPGMaker A2 tilesets (Those that only contain floor / floor details).
        // i.e. Inside_A2
        // This will also work with A1 tilesets, but those are meant to be animated.
        rpgmaker_a2,
        count
    } auto_type;

    Handle<assets::Texture> texture;
    std::string name;

    /// Returns the size of this tileset in tiles, taking the tilesize from tileset_manager::get_tile_size().
    [[nodiscard]] math::IVec2D get_size_in_tiles() const;

    /// Returns the size of this tileset in tiles, taking the tilesize as an argument.
    [[nodiscard]] math::IVec2D get_size_in_tiles(u32 tile_size) const;

    [[nodiscard]] math::Rect2D get_uv(u32 id) const;

    [[nodiscard]] math::Rect2D get_uv(u32 id, u32 tile_size) const;

    [[nodiscard]] u32 get_id(math::IVec2D pos) const;
    [[nodiscard]] u32 get_id_auto(u32 x_index, u32 surroundings) const;
    [[nodiscard]] u32 get_surroundings_from_auto_id(u32 id) const;
    [[nodiscard]] u32 get_x_index_from_auto_id(u32 id) const;
};

}

#endif // ARPIYI_TILESET_HPP
