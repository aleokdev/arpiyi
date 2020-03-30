#ifndef ARPIYI_MAP_HPP
#define ARPIYI_MAP_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "asset_manager.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "tileset.hpp"
#include "util/math.hpp"
#include "util/intdef.hpp"

namespace arpiyi_editor::assets {

struct Map {
    struct Tile {
        /// ID of tile (within layer tileset) being used
        u32 id;
    };

    class Layer {
    public:
        Layer() = delete;
        Layer(i64 width,
              i64 height,
              asset_manager::Handle<assets::Tileset> tileset);

        [[nodiscard]] Tile const& get_tile(math::IVec2D pos) const {
            return tiles[pos.x + pos.y * width];
        }
        void set_tile(math::IVec2D pos, Tile new_val) {
            tiles[pos.x + pos.y * width] = new_val;
            mesh.get()->destroy();
            *mesh.get() = generate_layer_split_quad();
        }
        [[nodiscard]] asset_manager::Handle<assets::Mesh> const get_mesh() const { return mesh; }

        asset_manager::Handle<assets::Tileset> tileset;
        std::string name;

    private:
        assets::Mesh generate_layer_split_quad();

        i64 width = 0, height = 0;
        std::vector<Tile> tiles;
        asset_manager::Handle<assets::Mesh> mesh;
    };

    std::vector<Layer> layers;
    std::string name;

    i64 width, height;
};

} // namespace arpiyi_editor::assets

#endif // ARPIYI_MAP_HPP
