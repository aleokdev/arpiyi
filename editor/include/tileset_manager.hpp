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
};

void init();

void render(bool* p_show);

void set_tile_size(u32);
u32 get_tile_size();

std::vector<Handle<assets::Tileset>> get_tilesets();

void set_selection_tileset(Handle<assets::Tileset>);
TilesetSelection const& get_selection();

} // namespace arpiyi::tileset_manager

#endif // ARPIYI_TILESET_MANAGER_HPP
