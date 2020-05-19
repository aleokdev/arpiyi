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
    rpgmaker_a2,
    count
};

template<TileType T> struct TileTypeData;

template<> struct TileTypeData<TileType::normal> {
    static constexpr aml::Vector2 tiles_per_represented_tile{1.f, 1.f};
};

struct [[assets::serialize]] [[meta::dir_name("tilesets")]] Tileset {
    TileType tile_type;

    Handle<assets::Texture> texture;
    std::string name;

    class Tile {
        Handle<Tileset> tileset;
        u64 tile_index;

        /// Returns a sprite with the full TilesetTile texture (Sized
        /// TileTypeData::tiles_per_represented_tile)
        [[nodiscard]] Sprite full_sprite() const;
        [[nodiscard]] PiecedSprite preview_sprite() const;

    private:
        template<TileType T> Sprite impl_full_sprite() const;
        template<TileType T> PiecedSprite impl_preview_sprite() const;
    };
    std::vector<Tile> all_tiles();

    math::IVec2D get_size_in_tiles();
};

struct TileInstance;
struct TileCustomSurroundings {
    bool override = false;
    /// All the neighbours this tile has. Set to true for letting the tile connect to that
    /// direction, false otherwise.
    bool down, down_right, right, up_right, up, up_left, left, down_left;
};
struct TileSurroundings {
    Tileset::Tile down, down_right, right, up_right, up, up_left, left, down_left;
};

/// An instance of Tileset::Tile. Can be serialized easily, as you only have to worry about its
/// surroundings and its parent TilesetTile.
struct TileInstance {
    Tileset::Tile parent;
    TileCustomSurroundings custom_surroundings;

    /// Returns a pieced sprite with the tile that corresponds the surroundings given. If
    /// custom_surroundings.override is set, the given surroundings will be completely ignored and
    /// the custom surroundings will be used instead.
    [[nodiscard]] PiecedSprite sprite(TileSurroundings const& surroundings) const;
};

template<> inline void raw_unload<Tileset>(Tileset& tileset) { tileset.texture.unload(); }

template<> struct LoadParams<Tileset> { fs::path path; };

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset);
template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params);

} // namespace arpiyi::assets

#endif // ARPIYI_TILESET_HPP
