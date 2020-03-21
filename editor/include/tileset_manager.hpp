#ifndef ARPIYI_TILESET_MANAGER_HPP
#define ARPIYI_TILESET_MANAGER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"

namespace arpiyi_editor::tileset_manager {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    unsigned int tile_size = 16;
    unsigned int vao = -1;
    unsigned int vbo = -1;

    math::IVec2D get_size_in_tiles() {
        return math::IVec2D{(int)texture.get()->w/(int)tile_size, (int)texture.get()->h/(int)tile_size};
    }
};

void init();

void render();

}

#endif // ARPIYI_TILESET_MANAGER_HPP
