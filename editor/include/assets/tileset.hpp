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
        // Used for RPGMaker A1 & A2 tilesets (Those that only contain floor / floor details).
        // i.e. Outside_A1 or Inside_A2
        rpgmaker_a12,
        count
    } auto_type;

    asset_manager::Handle<assets::Texture> texture;
    std::string name;

    /// Returns the size of this tileset in tiles, taking the tilesize from tileset_manager::get_tile_size().
    [[nodiscard]] math::IVec2D get_size_in_tiles() const;

    /// Returns the size of this tileset in tiles, taking the tilesize as an argument.
    [[nodiscard]] math::IVec2D get_size_in_tiles(u32 tile_size) const;

    [[nodiscard]] math::Rect2D get_uv(u32 id) const;

    [[nodiscard]] math::Rect2D get_uv(u32 id, u32 tile_size) const;

    [[nodiscard]] u32 get_id(math::IVec2D pos) const;
};

}

#endif // ARPIYI_TILESET_HPP
