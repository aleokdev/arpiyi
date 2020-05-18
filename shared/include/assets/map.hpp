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

/* clang-format off */
struct
[[assets::serialize]]
[[assets::load_before(Tileset)]]
[[assets::load_before(Entity)]]
[[meta::dir_name("maps")]] Map {
    /* clang-format on */
    struct Tile {
        /// ID of tile (within layer tileset) being used
        /// ID meaning depends on the tileset type. For information about each ID type, go to
        /// tileset.hpp::Tileset::AutoType.
        u32 id = 0;

        /// Terrain height of the tile.
        /// Can be positive or negative.
        i32 height = 0;

        enum class SlopeType : u8 {
            none,
            higher_y_means_higher_z,
            lower_y_means_higher_z,
            count
        };
        /// The direction this tile has higher Z in, if any.
        /// This is normally used to simulate stairs, walls, and basically anything that is
        /// vertical.
        SlopeType slope_type = SlopeType::none;

        /// If true, the mesh will include (not visible) quads to the sides (left, right and back)
        /// of the tile. This is done for simulating realistic shadows, and is mostly used for walls
        /// & solid terrain.
        /// Objects like furniture & semi-transparent tiles should normally set this to false.
        bool has_side_walls = true;
    };

    class Layer {
    public:
        Layer() = delete;
        Layer(i64 width, i64 height, Handle<assets::Tileset> tileset);

        [[nodiscard]] Tile& get_tile(math::IVec2D pos) {
            assert(is_pos_valid(pos));
            return tiles[pos.x + pos.y * width];
        }

        [[nodiscard]] Tile const& get_tile(math::IVec2D pos) const {
            assert(is_pos_valid(pos));
            return tiles[pos.x + pos.y * width];
        }

        [[nodiscard]] bool is_pos_valid(math::IVec2D pos) const {
            return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
        }

        /// TODO: Layer should not have mesh in it, this should be external
        [[nodiscard]] Handle<assets::Mesh> get_mesh() const { return mesh; }
        void regenerate_mesh();

        Handle<assets::Tileset> tileset;
        std::string name;
        bool visible = true;

    private:
        assets::Mesh generate_layer_mesh();
        template<bool is_auto> Mesh generate_mesh();

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
