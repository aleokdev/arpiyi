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
#include "entity.hpp"
#include "util/intdef.hpp"
#include "util/math.hpp"

namespace arpiyi_editor::assets {

struct Map {
    struct Tile {
        /// ID of tile (within layer tileset) being used
        u32 id = 0;
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

        [[nodiscard]] Handle<assets::Mesh> get_mesh() const { return mesh; }
        void regenerate_mesh();

        Handle<assets::Tileset> tileset;
        std::string name;
        bool visible = true;

    private:
        assets::Mesh generate_layer_split_quad();

        i64 width = 0, height = 0;
        std::vector<Tile> tiles;
        Handle<assets::Mesh> mesh;
    };

    struct Comment {
        std::string text;
        math::IVec2D pos;
    };

    std::vector<Handle<Layer>> layers;
    std::vector<Comment> comments;
    std::vector<Handle<Entity>> entities;
    std::string name;

    i64 width, height;
};

template<> inline void raw_unload<Map::Layer>(Map::Layer& layer) { }

template<> struct LoadParams<Map> { fs::path path; };

template<> RawSaveData raw_get_save_data<Map>(Map const&);
template<> void raw_load<Map>(Map&, LoadParams<Map> const&);

} // namespace arpiyi_editor::assets

#endif // ARPIYI_MAP_HPP
