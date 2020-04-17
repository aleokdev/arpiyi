#ifndef ARPIYI_EDITOR_MAP_MANAGER_HPP
#define ARPIYI_EDITOR_MAP_MANAGER_HPP

#include "asset_manager.hpp"
#include "assets/map.hpp"

namespace arpiyi::map_manager {

void init();
void render(bool* p_show);

Handle<assets::Map> get_current_map();

std::vector<Handle<assets::Map>> get_maps();

} // namespace arpiyi::map_manager

#endif // ARPIYI_EDITOR_MAP_MANAGER_HPP