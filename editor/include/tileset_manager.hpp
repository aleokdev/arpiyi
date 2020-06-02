#ifndef ARPIYI_TILESET_MANAGER_HPP
#define ARPIYI_TILESET_MANAGER_HPP

#include "asset_manager.hpp"
#include "assets/tileset.hpp"
#include "util/math.hpp"

#include <vector>

namespace arpiyi::tileset_manager {

struct TilesetSelection {
    Handle<assets::Tileset> tileset;
    math::IVec2D selection_start;
    math::IVec2D selection_end;

    struct Tile {
        math::IVec2D tileset_offset;
        assets::Tileset::Tile tile_ref;
    };
    std::vector<Tile> tiles_selected() const;
};

void init();

void render(bool* p_show);

std::vector<Handle<assets::Tileset>> get_tilesets();

void set_selection_tileset(Handle<assets::Tileset>);
TilesetSelection const& get_selection();

} // namespace arpiyi::tileset_manager

#endif // ARPIYI_TILESET_MANAGER_HPP
