#ifndef ARPIYI_MAP_HPP
#define ARPIYI_MAP_HPP

#include <cstdint>
#include <vector>
#include <string>

#include "texture.hpp"
#include "mesh.hpp"
#include "asset_manager.hpp"
#include "util/math.hpp"

namespace arpiyi_editor::assets {

struct Map {
    struct Tile {
        // Tile ID: 64 bits
        // First 16 bits (id & 0xFFFFFF): ID of tileset being used
        // Next 48 bits (id >> 16): ID of tile within tileset being used
        std::uint64_t id;
    };

    class Layer {
    public:
        Layer() {}
        Layer(std::size_t width, std::size_t height) : width(width), height(height), tiles(width * height) {}

        Tile& get_tile(math::IVec2D pos) {
            return tiles[pos.x + pos.y * width];
        }

        std::string name;
    private:
        std::size_t width = 0, height = 0;
        std::vector<Tile> tiles;
    };

    asset_manager::Handle<assets::Texture> texture_atlas;
    asset_manager::Handle<assets::Mesh> display_mesh;
    std::vector<Layer> layers;
    std::string name;

    std::size_t width;
    std::size_t height;
};

}

#endif // ARPIYI_MAP_HPP
