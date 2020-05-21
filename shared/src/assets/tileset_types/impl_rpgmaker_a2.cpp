#include "assets/map.hpp"
#include "assets/tileset.hpp"

#include <tuple>

namespace arpiyi::assets {

/// A "chunk" is a part of a tileset that represents a single tile. In normal tilesets, a single
/// tile is a chunk. In RPGMaker A2 tilesets, an area of 2x3 tiles (The chunk) is used to represent
/// a single automatic tile.
static constexpr int rpgmaker_a2_chunk_width = 2;
static constexpr int rpgmaker_a2_chunk_height = 3;

template<> Sprite Tileset::Tile::impl_full_sprite<TileType::rpgmaker_a2>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_normal_tiles = tl.size_in_tile_units();
    math::IVec2D size_in_rpgmaker_a2_tiles = {size_in_normal_tiles.x / rpgmaker_a2_chunk_width,
                                              size_in_normal_tiles.y / rpgmaker_a2_chunk_height};
    const u64 chunk_x = tile_index % size_in_rpgmaker_a2_tiles.x;
    const u64 chunk_y = tile_index / size_in_rpgmaker_a2_tiles.y;
    Sprite spr;
    Sprite::Piece piece;
    piece.source = {{static_cast<float>(chunk_x) / size_in_rpgmaker_a2_tiles.x,
                     static_cast<float>(chunk_y) / size_in_rpgmaker_a2_tiles.y},
                    {static_cast<float>(chunk_x + 1) / size_in_rpgmaker_a2_tiles.x,
                     static_cast<float>(chunk_y + 1) / size_in_rpgmaker_a2_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}

template<> Sprite Tileset::Tile::impl_preview_sprite<TileType::rpgmaker_a2>() const {
    // RPGMaker A2 tilesets use the first tile in a chunk to identify the preview.
    const auto& tl = *tileset.get();
    math::IVec2D size_in_normal_tiles = tl.size_in_tile_units();
    math::IVec2D size_in_rpgmaker_a2_tiles = {size_in_normal_tiles.x / rpgmaker_a2_chunk_width,
                                              size_in_normal_tiles.y / rpgmaker_a2_chunk_height};
    const u64 tile_x = tile_index % size_in_rpgmaker_a2_tiles.x;
    const u64 tile_y = tile_index / size_in_rpgmaker_a2_tiles.x;
    const float single_tile_width = 1.f / static_cast<float>(rpgmaker_a2_chunk_width);
    const float single_tile_height = 1.f / static_cast<float>(rpgmaker_a2_chunk_height);
    Sprite spr;
    Sprite::Piece piece;
    piece.source = {
        {static_cast<float>(tile_x) / size_in_rpgmaker_a2_tiles.x,
         static_cast<float>(tile_y) / size_in_rpgmaker_a2_tiles.y},
        {(static_cast<float>(tile_x) + single_tile_width) / size_in_rpgmaker_a2_tiles.x,
         (static_cast<float>(tile_y) + single_tile_height) / size_in_rpgmaker_a2_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}
template<> std::size_t Tileset::impl_tile_count<TileType::rpgmaker_a2>() const {
    math::IVec2D size_in_normal_tiles = size_in_tile_units();
    math::IVec2D size_in_rpgmaker_a2_tiles = {size_in_normal_tiles.x / rpgmaker_a2_chunk_width,
                                              size_in_normal_tiles.y / rpgmaker_a2_chunk_height};
    return size_in_rpgmaker_a2_tiles.x * size_in_rpgmaker_a2_tiles.y;
}
template<>
Sprite Map::Tile::impl_sprite<TileType::rpgmaker_a2>(Layer const& this_layer, math::IVec2D this_pos) const {
    assert(parent.tileset.get());
    math::IVec2D size_in_normal_tiles = parent.tileset.get()->size_in_tile_units();
    math::IVec2D size_in_rpgmaker_a2_tiles = {size_in_normal_tiles.x / rpgmaker_a2_chunk_width,
                                              size_in_normal_tiles.y / rpgmaker_a2_chunk_height};
    const u64 chunk_x = parent.tile_index % size_in_rpgmaker_a2_tiles.x;
    const u64 chunk_y = parent.tile_index / size_in_rpgmaker_a2_tiles.x;
    const auto connections = calculate_connections(this_layer.get_surroundings(this_pos));

    /* clang-format off */
    /// Where each minitile is located locally in the RPGMaker A2 layout.
    /// Explanation: https://imgur.com/a/vlRJ9cY
    const std::array<math::IVec2D, 20> layout = {
        /* A1 */ math::IVec2D{2, 0},
        /* A2 */ math::IVec2D{0, 2},
        /* A3 */ math::IVec2D{2, 4},
        /* A4 */ math::IVec2D{2, 2},
        /* A5 */ math::IVec2D{0, 4},
        /* B1 */ math::IVec2D{3, 0},
        /* B2 */ math::IVec2D{3, 2},
        /* B3 */ math::IVec2D{1, 4},
        /* B4 */ math::IVec2D{1, 2},
        /* B5 */ math::IVec2D{3, 4},
        /* C1 */ math::IVec2D{2, 1},
        /* C2 */ math::IVec2D{0, 5},
        /* C3 */ math::IVec2D{2, 3},
        /* C4 */ math::IVec2D{2, 5},
        /* C5 */ math::IVec2D{0, 3},
        /* D1 */ math::IVec2D{3, 1},
        /* D2 */ math::IVec2D{3, 5},
        /* D3 */ math::IVec2D{1, 3},
        /* D4 */ math::IVec2D{1, 5},
        /* D5 */ math::IVec2D{3, 3}
    };
    /* clang-format on */

    Sprite spr;
    for (int minitile = 0; minitile < 4; ++minitile) {
        bool is_connected_vertically, is_connected_horizontally, is_connected_via_corner;
        /* clang-format off */
        std::array<std::tuple<bool, bool, bool>, 4> conn{
            std::tuple{connections.up, connections.left, connections.up_left}, // Top-left minitile
            std::tuple{connections.up, connections.right, connections.up_right}, // Top-right minitile
            std::tuple{connections.down, connections.left, connections.down_left}, // Bottom-left minitile
            std::tuple{connections.down, connections.right, connections.down_right} // Bottom-right minitile
        };
        /* clang-format on */
        is_connected_vertically = std::get<0>(conn[minitile]);
        is_connected_horizontally = std::get<1>(conn[minitile]);
        is_connected_via_corner = std::get<2>(conn[minitile]);

        u8 layout_index = minitile * 5; // Set the minitile position: AX, BX, CX, DX
        switch ((is_connected_via_corner << 2u) | (is_connected_vertically << 1u) |
                (is_connected_horizontally)) {
            case (0b011):
                /* layout_index += 0; */ // X1; All connected except corner
                break;
            case (0b100):
            case (0b000):
                layout_index += 1; // X2; No connections
                break;
            case (0b111):
                layout_index += 2; // X3; All connected
                break;
            case (0b101):
            case (0b001):
                layout_index += 3; // X4; Vertical connection
                break;
            case (0b110):
            case (0b010):
                layout_index += 4; // X5; Horizontal connection
                break;
        }
        const float single_tile_width = 1.f / static_cast<float>(rpgmaker_a2_chunk_width);
        const float single_tile_height = 1.f / static_cast<float>(rpgmaker_a2_chunk_height);

        Sprite::Piece piece;
        piece.source = {
            {static_cast<float>(chunk_x) / size_in_rpgmaker_a2_tiles.x +
                 layout[layout_index].x / 2.f / size_in_normal_tiles.x,
             static_cast<float>(chunk_y) / size_in_rpgmaker_a2_tiles.y +
                 layout[layout_index].y / 2.f / size_in_normal_tiles.y},
            {(static_cast<float>(chunk_x) + single_tile_width / 2.f) / size_in_rpgmaker_a2_tiles.x +
                 layout[layout_index].x / 2.f / size_in_normal_tiles.x,
             (static_cast<float>(chunk_y) + single_tile_height / 2.f) / size_in_rpgmaker_a2_tiles.y +
                 layout[layout_index].y / 2.f / size_in_normal_tiles.y}};
        piece.destination = {
            {static_cast<float>(minitile % 2) / 2.f, (1.f-static_cast<float>(minitile / 2)) / 2.f},
            {static_cast<float>(minitile % 2) / 2.f + .5f, (1.f-static_cast<float>(minitile / 2)) / 2.f + .5f}};
        spr.pieces.emplace_back(Sprite::Piece{piece});
    }

    return spr;
}

} // namespace arpiyi::assets