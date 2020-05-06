#ifndef ARPIYI_MAP_HPP
#define ARPIYI_MAP_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "asset_manager.hpp"
#include "entity.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "tileset.hpp"
#include "util/intdef.hpp"
#include "util/math.hpp"

namespace arpiyi::assets {

struct [[assets::serialize]] [[assets::load_before(Tileset)]] [[assets::load_before(Entity)]] [[meta::dir_name("maps")]] Map {
    struct Tile {
        /// ID of tile (within layer tileset) being used
        /// ID meaning depends on the tileset type. For information about each ID type, go to tileset.hpp::Tileset::AutoType.
        u32 id = 0;

        /// Terrain height of the tile.
        /// Can be positive or negative. If height == wall_height, this tile is a wall.
        i32 height = 0;
        constexpr static i32 wall_height = -999;
    };

    class Layer {
    public:
        Layer() = delete;
        Layer(i64 width, i64 height, Handle<assets::Tileset> tileset);

        [[nodiscard]] Tile get_tile(math::IVec2D pos) const {
            assert(is_pos_valid(pos));
            return tiles[pos.x + pos.y * width];
        }

        [[nodiscard]] bool is_pos_valid(math::IVec2D pos) const {
            return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
        }

        void set_tile(math::IVec2D pos, Tile new_val) {
            tiles[pos.x + pos.y * width] = new_val;
            regenerate_mesh();
        }

        /// TODO: Layer should not have mesh in it, this should be external
        [[nodiscard]] Handle<assets::Mesh> get_mesh() const { return mesh; }
        void regenerate_mesh();

        Handle<assets::Tileset> tileset;
        std::string name;
        bool visible = true;

    private:
        assets::Mesh generate_layer_mesh();
        assets::Mesh generate_normal_mesh();
        assets::Mesh generate_rpgmaker_a2_mesh();

        i64 width = 0, height = 0;
        std::vector<Tile> tiles;
        Handle<assets::Mesh> mesh;
    };

    struct Comment {
        std::string text;
        math::IVec2D pos;
    };

    std::vector<Handle<Layer>> layers;
    std::vector<Handle<Comment>> comments;
    std::vector<Handle<Entity>> entities;
    std::string name;

    i64 width, height;
};

template<> inline void raw_unload<Map::Layer>(Map::Layer&) {}
template<> inline void raw_unload<Map::Comment>(Map::Comment&) {}

template<> struct LoadParams<Map> { fs::path path; };

template<> RawSaveData raw_get_save_data<Map>(Map const&);
template<> void raw_load<Map>(Map&, LoadParams<Map> const&);

} // namespace arpiyi::assets

#endif // ARPIYI_MAP_HPP
