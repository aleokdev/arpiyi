#ifndef ARPIYI_TILESET_MANAGER_HPP
#define ARPIYI_TILESET_MANAGER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"
#include "assets/tileset.hpp"

#include <vector>

namespace arpiyi_editor::tileset_manager {

void init();

void render();

std::size_t get_tile_size();

std::vector<asset_manager::Handle<assets::Tileset>>& get_tilesets();

}

#endif // ARPIYI_TILESET_MANAGER_HPP
