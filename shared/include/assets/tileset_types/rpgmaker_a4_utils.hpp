//
// Created by aleok on 2/6/20.
//

#ifndef ARPIYI_RPGMAKER_A4_UTILS_HPP
#define ARPIYI_RPGMAKER_A4_UTILS_HPP

#include "assets/map.hpp"
#include "assets/sprite.hpp"

namespace arpiyi::tileset_types {

/// Calculates a RPGMaker A4 wall tile. The "connections" parameter only takes into account the up,
/// down, left and right directions.
assets::Sprite calculate_rpgmaker_a4_wall_tile(assets::TextureChunk const& chunk,
                                               assets::Map::TileConnections connections);

} // namespace arpiyi::tileset_types

#endif // ARPIYI_RPGMAKER_A4_UTILS_HPP
