#ifndef ARPIYI_INSPECTOR_HPP
#define ARPIYI_INSPECTOR_HPP

#include "assets/entity.hpp"
#include "assets/map.hpp"

namespace arpiyi::widgets::inspector {

void init();

void render(bool* p_open);

void set_inspected_asset(Handle<assets::Entity> entity);
void set_inspected_asset(Handle<assets::Map::Comment> comment);

} // namespace arpiyi::widgets::inspector

#endif // ARPIYI_INSPECTOR_HPP
