#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "texture.hpp"
#include "mesh.hpp"

namespace arpiyi_editor::assets {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    unsigned int tile_size = 48;
    asset_manager::Handle<assets::Mesh> display_mesh;

    math::IVec2D get_size_in_tiles() {
        return math::IVec2D{(int)texture.get()->w/(int)tile_size, (int)texture.get()->h/(int)tile_size};
    }
};

}

#endif // ARPIYI_TILESET_HPP
