#ifndef ARPIYI_RPGMAKER_A2_UTILS_HPP
#define ARPIYI_RPGMAKER_A2_UTILS_HPP

#include "util/math.hpp"
#include "assets/map.hpp"
#include "assets/sprite.hpp"
#include "renderer/renderer.hpp"

namespace arpiyi::tileset_types {

assets::Sprite calculate_rpgmaker_a2_tile(assets::TextureChunk const& chunk, assets::Map::TileConnections connections);

}

#endif // ARPIYI_RPGMAKER_A2_UTILS_HPP
