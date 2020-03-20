#ifndef ARPIYI_TILESET_MANAGER_HPP
#define ARPIYI_TILESET_MANAGER_HPP

#include "asset_manager.hpp"

namespace arpiyi_editor::tileset_manager {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    unsigned int tile_size;
};

void init();

void render();

}

#endif // ARPIYI_TILESET_MANAGER_HPP
