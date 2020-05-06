#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "util/intdef.hpp"
#include "util/math.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

namespace arpiyi::assets {

struct [[assets::serialize]] [[meta::dir_name("tilesets")]] Tileset {
    enum class AutoType {
        /// The ID of a tile in a layer with a tileset using the none AutoType is:
        /// tileset_tile.x + tileset_tile.y * tileset.width.
        none,
        /// Used for RPGMaker A2 tilesets (Those that only contain floor / floor details).
        /// i.e. Inside_A2
        /// This will also work with A1 tilesets, but those are meant to be animated.
        ///
        /// The ID of a tile in a layer with a tileset using the rpgmaker_a2 AutoType is:
        /// neighbour_bitfield + auto_tile_index << 8.
        /// Where neighbour_bitfield is an 8-bit integer describing the connection with the tile's neighbours:
        /// \neighbour_bitfield
        /// 0 | 0 | 0\neighbour_bitfield
        /// 0 | x | 0\neighbour_bitfield
        /// 0 | 0 | 0\neighbour_bitfield
        /// Where x is the ID owner (the tile) and the zeros are the neighbours/bits of the neighbour_bitfield.
        rpgmaker_a2,
        count
    } auto_type;

    Handle<assets::Texture> texture;
    std::string name;

    /// Returns the size of this tileset in tiles.
    [[nodiscard]] math::IVec2D get_size_in_tiles() const;

    // TODO: REFACTORME
    // This is a mess

    [[nodiscard]] math::Rect2D get_uv(u32 id) const;
    [[nodiscard]] math::Rect2D get_uv(u32 id, u8 minitile) const;

    [[nodiscard]] u32 get_id_autotype_none(math::IVec2D pos) const;
    [[nodiscard]] u32 get_id_autotype_rpgmaker_a2(u32 auto_tile_index, u8 surroundings) const;
    [[nodiscard]] u8 get_surroundings_from_auto_id(u32 id) const;
    [[nodiscard]] u32 get_auto_tile_index_from_auto_id(u32 id) const;
    [[nodiscard]] u32 get_auto_tile_count() const;
};

template<> inline void raw_unload<Tileset>(Tileset& tileset) { tileset.texture.unload(); }

template<> struct LoadParams<Tileset> { fs::path path; };

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset);
template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params);

} // namespace arpiyi::assets

#endif // ARPIYI_TILESET_HPP
