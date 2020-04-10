#ifndef ARPIYI_EDITOR_MAP_MANAGER_HPP
#define ARPIYI_EDITOR_MAP_MANAGER_HPP

#include "assets/map.hpp"
#include "asset_manager.hpp"

namespace arpiyi_editor::map_manager {

void init();
void render(bool* p_show);

Handle<assets::Map> get_current_map();

std::vector<Handle<assets::Map>> get_maps();

}

#endif // ARPIYI_EDITOR_MAP_MANAGER_HPP