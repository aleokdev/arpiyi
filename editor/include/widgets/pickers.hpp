#ifndef ARPIYI_PICKERS_HPP
#define ARPIYI_PICKERS_HPP

#include "asset_manager.hpp"
#include "assets/sprite.hpp"
#include "assets/tileset.hpp"

namespace arpiyi::widgets {

namespace tileset_picker {

void show(Handle<assets::Tileset>& selected_tileset);

}

namespace sprite_picker {

// Shows the sprite picker (Not a window, but the widget contents) Returns true if sprite given was
// changed.
bool show(Handle<assets::Sprite>& selected_sprite);

} // namespace sprite_picker

} // namespace arpiyi::widgets

#endif // ARPIYI_PICKERS_HPP
