#include "assets/tileset.hpp"
#include "assets/map.hpp"

namespace arpiyi::assets {

template<> Sprite Tileset::Tile::impl_full_sprite<TileType::normal>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_tiles = tl.size_in_tile_units();
    const int tile_x = tile_index % size_in_tiles.x;
    const int tile_y = tile_index / size_in_tiles.x;
    Sprite spr;
    Sprite::Piece piece;
    piece.source = {{static_cast<float>(tile_x) / size_in_tiles.x,
                        static_cast<float>(tile_y) / size_in_tiles.y},
                    {static_cast<float>(tile_x + 1) / size_in_tiles.x,
                        static_cast<float>(tile_y + 1) / size_in_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}

template<> Sprite Tileset::Tile::impl_preview_sprite<TileType::normal>() const {
    const auto& tl = *tileset.get();
    math::IVec2D size_in_tiles = tl.size_in_tile_units();
    const int tile_x = tile_index % size_in_tiles.x;
    const int tile_y = tile_index / size_in_tiles.x;
    Sprite spr;
    Sprite::Piece piece;
    piece.source = {{static_cast<float>(tile_x) / size_in_tiles.x,
                     static_cast<float>(tile_y) / size_in_tiles.y},
                    {static_cast<float>(tile_x + 1) / size_in_tiles.x,
                     static_cast<float>(tile_y + 1) / size_in_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}

template<> std::size_t Tileset::impl_tile_count<TileType::normal>() const {
    const math::IVec2D size = size_in_tile_units();
    return size.x * size.y;
}

template<> Sprite Map::Tile::impl_sprite<TileType::normal>(Layer const& this_layer, math::IVec2D this_pos) const {
    const auto& tl = *parent.tileset.get();
    math::IVec2D size_in_tiles = tl.size_in_tile_units();
    const int tile_x = parent.tile_index % size_in_tiles.x;
    const int tile_y = parent.tile_index / size_in_tiles.x;
    Sprite spr;
    Sprite::Piece piece;
    piece.source = {{static_cast<float>(tile_x) / size_in_tiles.x,
                        static_cast<float>(tile_y) / size_in_tiles.y},
                    {static_cast<float>(tile_x + 1) / size_in_tiles.x,
                        static_cast<float>(tile_y + 1) / size_in_tiles.y}};
    piece.destination = {{0, 0}, {1, 1}};
    spr.pieces = std::vector<Sprite::Piece>{piece};
    return spr;
}

} // namespace arpiyi::assets