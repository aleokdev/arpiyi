#ifndef ARPIYI_INSPECTOR_HPP
#define ARPIYI_INSPECTOR_HPP

#include "assets/entity.hpp"
#include "assets/map.hpp"

namespace arpiyi_editor::widgets::inspector {

void init();

void render(bool* p_open);

void set_inspected_asset(Handle<assets::Entity> entity);
void set_inspected_asset(Handle<assets::Map::Comment> comment);

}

#endif // ARPIYI_INSPECTOR_HPP
