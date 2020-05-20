#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "mesh.hpp"
#include "sprite.hpp"
#include "texture.hpp"
#include "util/intdef.hpp"
#include "util/math.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <anton/math/vector2.hpp>

namespace aml = anton::math;
namespace fs = std::filesystem;

namespace arpiyi::assets {

enum class TileType {
    /// The ID of a tile in a layer with a tileset using the normal AutoType is:
    /// tileset_tile.x + tileset_tile.y * tileset.width.
    normal,
    /// Used for RPGMaker A2 tilesets (Those that only contain floor / floor details).
    /// i.e. Inside_A2
    /// This will also work with A1 tilesets, but those are meant to be animated.
    ///
    /// The ID of a tile in a layer with a tileset using the rpgmaker_a2 AutoType is:
    /// neighbour_bitfield + auto_tile_index << 8.
    /// Where neighbour_bitfield is an 8-bit integer describing the connection with the tile's
    /// neighbours:
    /// \neighbour_bitfield
    /// 0 | 0 | 0\neighbour_bitfield
    /// 0 | x | 0\neighbour_bitfield
    /// 0 | 0 | 0\neighbour_bitfield
    /// Where x is the ID owner (the tile) and the zeros are the neighbours/bits of the
    /// neighbour_bitfield.
    // TODO: Reimplement RPGMaker A2 tilesets
    // rpgmaker_a2,
    count
};

struct [[assets::serialize]] [[meta::dir_name("tilesets")]] Tileset {
    TileType tile_type;

    Handle<assets::Texture> texture;
    std::string name;

    struct Tile {
        Handle<Tileset> tileset;
        u64 tile_index;

        /// Returns a sprite with the full TilesetTile texture (Sized
        /// TileTypeData::tiles_per_represented_tile)
        [[nodiscard]] Sprite full_sprite() const;
        [[nodiscard]] Sprite preview_sprite() const;

    private:
        template<TileType T> Sprite impl_full_sprite() const;
        template<TileType T> Sprite impl_preview_sprite() const;
    };
    /// Returns how many Tileset::Tiles are present in this tileset.
    [[nodiscard]] std::size_t tile_count() const;

    /// Returns the size in *actual* tiles, not Tileset::Tile tiles. This is literally just dividing
    /// the texture size by the global tile size.
    [[nodiscard]] math::IVec2D size_in_tile_units() const;

private:
    template<TileType T>[[nodiscard]] std::size_t impl_tile_count() const;
};

template<> inline void raw_unload<Tileset>(Tileset& tileset) { }

template<> struct LoadParams<Tileset> { fs::path path; };

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset);
template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params);

} // namespace arpiyi::assets

#endif // ARPIYI_TILESET_HPP
