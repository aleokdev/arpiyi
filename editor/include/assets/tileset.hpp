#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "texture.hpp"

namespace arpiyi_editor::assets {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    unsigned int tile_size = 48;
    unsigned int vao = -1;
    unsigned int vbo = -1;

    math::IVec2D get_size_in_tiles() {
        return math::IVec2D{(int)texture.get()->w/(int)tile_size, (int)texture.get()->h/(int)tile_size};
    }
};

}

#endif // ARPIYI_TILESET_HPP
