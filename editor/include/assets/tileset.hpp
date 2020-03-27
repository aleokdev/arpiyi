#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "texture.hpp"
#include "mesh.hpp"
#include "util/math.hpp"

#include <string>

namespace arpiyi_editor::assets {

struct Tileset {
    asset_manager::Handle<assets::Texture> texture;
    std::string name;

    math::IVec2D get_size_in_tiles(std::size_t tile_size) {
        auto tex = texture.get();
        return math::IVec2D{(int)tex->w/(int)tile_size, (int)tex->h/(int)tile_size};
    }
};

}

#endif // ARPIYI_TILESET_HPP
