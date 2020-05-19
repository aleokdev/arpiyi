#include "assets/tileset.hpp"

namespace arpiyi::assets {

template<> Sprite Tileset::Tile::impl_full_sprite<TileType::normal>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_tiles = tl.size_in_tile_units();
    const int tile_x = tile_index % size_in_tiles.x;
    const int tile_y = tile_index / size_in_tiles.x;
    Sprite spr;
    spr.texture = tl.texture;
    spr.uv_min = {static_cast<float>(tile_x) / size_in_tiles.x,
                  static_cast<float>(tile_y) / size_in_tiles.y};
    spr.uv_max = {static_cast<float>(tile_x + 1) / size_in_tiles.x,
                  static_cast<float>(tile_y + 1) / size_in_tiles.y};
    return spr;
}

template<> PiecedSprite Tileset::Tile::impl_preview_sprite<TileType::normal>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_tiles = tl.size_in_tile_units();
    const int tile_x = tile_index % size_in_tiles.x;
    const int tile_y = tile_index / size_in_tiles.x;
    PiecedSprite spr;
    PiecedSprite::Piece piece;
    piece.source = {{static_cast<float>(tile_x) / size_in_tiles.x,
                     static_cast<float>(tile_y) / size_in_tiles.y},
                    {static_cast<float>(tile_x + 1) / size_in_tiles.x,
                     static_cast<float>(tile_y + 1) / size_in_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<PiecedSprite::Piece>{piece};
    return spr;
}

template<> std::size_t Tileset::impl_tile_count<TileType::normal>() const {
    const math::IVec2D size = size_in_tile_units();
    return size.x * size.y;
}

} // namespace arpiyi::assets