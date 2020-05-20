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
    struct TileConnections {
        /// All the neighbours this tile has. Set to true for letting the tile connect to that
        /// direction, false otherwise.
        bool down, down_right, right, up_right, up, up_left, left, down_left;
    };
    struct TileSurroundings {
        Tileset::Tile const *down, *down_right, *right, *up_right, *up, *up_left, *left, *down_left;
    };

    /// An instance of Tileset::Tile that also contains other information such as height, slope
    /// type, etc.
    struct Tile {
        bool exists = false;

        Tileset::Tile parent;
        bool override_connections = false;
        TileConnections custom_connections;

        // Terrain height of the tile.
        /// Can be positive or negative.
        i32 height = 0;

        enum class SlopeType : u8 { none, higher_y_means_higher_z, lower_y_means_higher_z, count };
        /// The direction this tile has higher Z in, if any.
        /// This is normally used to simulate stairs, walls, and basically anything that is
        /// vertical.
        SlopeType slope_type = SlopeType::none;

        /// If true, the mesh will include (not visible) quads to the sides (left, right and back)
        /// of the tile. This is done for simulating realistic shadows, and is mostly used for walls
        /// & solid terrain.
        /// Objects like furniture & semi-transparent tiles should normally set this to false.
        bool has_side_walls = true;

        /// Returns a pieced sprite with the tile that corresponds the surroundings given. If
        /// override_connections is set, the given surroundings will be completely ignored and
        /// custom_connections will be used instead.
        [[nodiscard]] Sprite sprite(TileSurroundings const& surroundings) const;

    private:
        /// This will return custom_connections if override_connections is set to true.
        TileConnections calculate_connections(TileSurroundings const& surroundings) const;
        template<TileType T> Sprite impl_sprite(TileSurroundings const& surroundings) const;
    };

    class Layer {
    public:
        Layer() = delete;
        Layer(i64 width, i64 height);

        [[nodiscard]] Tile& get_tile(math::IVec2D pos) {
            assert(is_pos_valid(pos));
            return tiles[pos.x + pos.y * width];
        }

        [[nodiscard]] TileSurroundings get_surroundings(math::IVec2D pos) const;

        [[nodiscard]] Tile const& get_tile(math::IVec2D pos) const {
            assert(is_pos_valid(pos));
            return tiles[pos.x + pos.y * width];
        }

        [[nodiscard]] bool is_pos_valid(math::IVec2D pos) const {
            return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
        }

        std::string name;
        bool visible = true;

    private:
        i64 width = 0, height = 0;
        std::vector<Tile> tiles;
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
