#include "assets/map.hpp"
#include "assets/tileset.hpp"
#include "assets/tileset_types/rpgmaker_a2_utils.hpp"
#include "assets/tileset_types/rpgmaker_a4_utils.hpp"

#include <array>

namespace {

/// A "chunk" is a part of a tileset that represents a single tile. In normal tilesets, a single
/// tile is a chunk. In RPGMaker A4 tilesets, an area of 2x3 tiles (The chunk) is used to represent
/// a single automatic ceiling, and below it an area of 2x2 tiles is a single wall chunk.
static constexpr int chunk_width = 2;
static constexpr int ceiling_chunk_height = 3;
static constexpr int wall_chunk_height = 2;

} // namespace
namespace arpiyi::assets {

template<> Sprite Tileset::Tile::impl_full_sprite<TileType::rpgmaker_a4>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_normal_tiles = tl.size_in_tile_units();
    const u64 chunk_x = tile_index % (size_in_normal_tiles.x / chunk_width);
    const u64 chunk_y = tile_index / (size_in_normal_tiles.x / chunk_width);

    u64 start_tile_y = 0;
    for (u64 i_chunk_y = 0; i_chunk_y < chunk_y; ++i_chunk_y) {
        if (i_chunk_y % 2 == 0)
            start_tile_y += ceiling_chunk_height;
        else
            start_tile_y += wall_chunk_height;
    }
    Sprite spr;
    Sprite::Piece piece;
    const bool is_ceiling = chunk_y % 2 == 0;
    piece.source = {
        {static_cast<float>(chunk_x) / static_cast<float>(size_in_normal_tiles.x / chunk_width),
         static_cast<float>(start_tile_y) / size_in_normal_tiles.y},
        {static_cast<float>(chunk_x + 1) / static_cast<float>(size_in_normal_tiles.x / chunk_width),
         static_cast<float>(start_tile_y +
                            (is_ceiling ? ceiling_chunk_height : wall_chunk_height)) /
             size_in_normal_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}

template<> Sprite Tileset::Tile::impl_preview_sprite<TileType::rpgmaker_a4>() const {
    // RPGMaker A4 tilesets use the first tile in a ceiling to identify the ceiling preview.
    // As for the wall, we can use the four outer corners of the 2x2 wall chunk to create a preview.
    const auto& tl = *tileset.get();
    math::IVec2D size_in_normal_tiles = tl.size_in_tile_units();
    const u64 chunk_x = tile_index % (size_in_normal_tiles.x / chunk_width);
    const u64 chunk_y = tile_index / (size_in_normal_tiles.x / chunk_width);

    const u64 start_tile_x = chunk_x * chunk_width;
    u64 start_tile_y = 0;
    for (u64 i_chunk_y = 0; i_chunk_y < chunk_y; ++i_chunk_y) {
        if (i_chunk_y % 2 == 0)
            start_tile_y += ceiling_chunk_height;
        else
            start_tile_y += wall_chunk_height;
    }
    Sprite spr;
    const bool is_ceiling = chunk_y % 2 == 0;
    if (is_ceiling) {
        Sprite::Piece piece;
        piece.source = {
            {static_cast<float>(start_tile_x) / static_cast<float>(size_in_normal_tiles.x),
             static_cast<float>(start_tile_y) / size_in_normal_tiles.y},
            {static_cast<float>(start_tile_x + 1.f) / static_cast<float>(size_in_normal_tiles.x),
             static_cast<float>(start_tile_y + 1) / size_in_normal_tiles.y}};
        piece.destination = {{0, 0}, {1, 1}};
        spr.pieces = std::vector<Sprite::Piece>{piece};
    } else {
        // Form a single sprite from the 4 outer corners of the wall
        /// The positions of the corner minitiles, starting from the chunk starting pos, measured in
        /// tiles.
        constexpr std::array<aml::Vector2, 4> corner_positions = {
            aml::Vector2{0, 0}, aml::Vector2{1.5f, 0}, aml::Vector2{0, 1.5f},
            aml::Vector2{1.5f, 1.5f}};
        for (int minitile = 0; minitile < 4; ++minitile) {
            Sprite::Piece piece;
            aml::Vector2 minitile_pos =
                aml::Vector2(start_tile_x, start_tile_y) + corner_positions[minitile];
            piece.source = {{minitile_pos.x / static_cast<float>(size_in_normal_tiles.x),
                             minitile_pos.y / size_in_normal_tiles.y},
                            {(minitile_pos.x + 0.5f) / static_cast<float>(size_in_normal_tiles.x),
                             (minitile_pos.y + 0.5f) / size_in_normal_tiles.y}};
            piece.destination = {{static_cast<float>(minitile % 2) / 2.f,
                                  (1 - static_cast<float>(minitile / 2)) / 2.f},
                                 {static_cast<float>(minitile % 2) / 2.f + 0.5f,
                                  (1 - static_cast<float>(minitile / 2)) / 2.f + 0.5f}};
            spr.pieces.emplace_back(piece);
        }
    }

    return spr;
}
template<> std::size_t Tileset::impl_tile_count<TileType::rpgmaker_a4>() const {
    math::IVec2D size_in_normal_tiles = size_in_tile_units();
    u64 y_chunk_count = 0;
    for (u64 i_tile_y = 0; i_tile_y < size_in_normal_tiles.y; ++y_chunk_count) {
        if (y_chunk_count % 2 == 0)
            i_tile_y += ceiling_chunk_height;
        else
            i_tile_y += wall_chunk_height;
    }
    math::IVec2D size_in_rpgmaker_a4_tiles = {size_in_normal_tiles.x / chunk_width,
                                              static_cast<i32>(y_chunk_count)};
    return size_in_rpgmaker_a4_tiles.x * size_in_rpgmaker_a4_tiles.y;
}
template<>
Sprite Map::Tile::impl_sprite<TileType::rpgmaker_a4>(Layer const& this_layer,
                                                     math::IVec2D this_pos) const {
    assert(parent.tileset.get());
    assert(parent.tileset.get()->texture.get());
    math::IVec2D size_in_normal_tiles = parent.tileset.get()->size_in_tile_units();
    const u64 chunk_x = parent.tile_index % (size_in_normal_tiles.x / chunk_width);
    const u64 chunk_y = parent.tile_index / (size_in_normal_tiles.x / chunk_width);

    const u64 start_tile_x = chunk_x * chunk_width;
    u64 start_tile_y = 0;
    for (u64 i_chunk_y = 0; i_chunk_y < chunk_y; ++i_chunk_y) {
        if (i_chunk_y % 2 == 0)
            start_tile_y += ceiling_chunk_height;
        else
            start_tile_y += wall_chunk_height;
    }
    bool is_ceiling = chunk_y % 2 == 0;

    if (is_ceiling) {
        assets::TextureChunk chunk{
            parent.tileset.get()->texture.get()->handle,
            {{(float)start_tile_x / (float)size_in_normal_tiles.x,
              (float)start_tile_y / (float)size_in_normal_tiles.y},
             {(float)(start_tile_x + chunk_width) / (float)size_in_normal_tiles.x,
              (float)(start_tile_y + ceiling_chunk_height) / (float)size_in_normal_tiles.y}}};
        const auto connections = calculate_connections(this_layer.get_surroundings(this_pos));
        return tileset_types::calculate_rpgmaker_a2_tile(chunk, connections);
    } else {
        Map::TileConnections connections;
        if(override_connections) connections = custom_connections;
        else {
            struct WallCount {
                int above = -1;
                int below = -1;
            };
            const auto count_walls = [&](int x_offset) -> WallCount {
              WallCount wallcount;
              while(this_layer.is_pos_valid({this_pos.x + x_offset, this_pos.y + wallcount.above + 1})) {
                  const auto& above_tile = this_layer.get_tile({this_pos.x + x_offset, this_pos.y + wallcount.above + 1});
                  if(above_tile.parent.tile_index == parent.tile_index && above_tile.parent.tileset.get_id() == parent.tileset.get_id())
                      wallcount.above++;
                  else break;
              }
              while(this_layer.is_pos_valid({this_pos.x + x_offset, this_pos.y - wallcount.below - 1})) {
                  const auto& below_tile = this_layer.get_tile({this_pos.x + x_offset, this_pos.y - wallcount.below - 1});
                  if(below_tile.parent.tile_index == parent.tile_index && below_tile.parent.tileset.get_id() == parent.tileset.get_id())
                      wallcount.below++;
                  else break;
              }
              return wallcount;
            };

            WallCount this_wallcount = count_walls(0);
            WallCount left_wallcount = count_walls(-1);
            WallCount right_wallcount = count_walls(1);
            connections = {
                this_wallcount.below != 0,
                false,
                right_wallcount.above == this_wallcount.above &&  right_wallcount.below == this_wallcount.below,
                false,
                this_wallcount.above != 0,
                false,
                left_wallcount.above == this_wallcount.above &&  left_wallcount.below == this_wallcount.below,
                false
            };
        }

        assets::TextureChunk chunk{
            parent.tileset.get()->texture.get()->handle,
            {{(float)start_tile_x / (float)size_in_normal_tiles.x,
              (float)start_tile_y / (float)size_in_normal_tiles.y},
             {(float)(start_tile_x + chunk_width) / (float)size_in_normal_tiles.x,
              (float)(start_tile_y + wall_chunk_height) / (float)size_in_normal_tiles.y}}};
        return tileset_types::calculate_rpgmaker_a4_wall_tile(chunk, connections);
    }
}

} // namespace arpiyi::assets

namespace arpiyi::tileset_types {

assets::Sprite calculate_rpgmaker_a4_wall_tile(assets::TextureChunk const& chunk,
                                               assets::Map::TileConnections connections) {
    // The wall tile can be calculated with the same algorithm as a regular RPGMaker A2 tile, but
    // you cannot use inner corners because wall tiles are always convex.
    connections.up_right = connections.down_left = connections.down_right = connections.up_left =
        true;
    assets::TextureChunk new_chunk = chunk;
    // Include the upper tile that the RPGMaker A2 autotiling algorithm needs. We aren't going to
    // use it because we don't have inner corners.
    new_chunk.rect.start.y -= (new_chunk.rect.end.y - new_chunk.rect.start.y) / wall_chunk_height;
    return tileset_types::calculate_rpgmaker_a2_tile(new_chunk, connections);
}

} // namespace arpiyi::tileset_types