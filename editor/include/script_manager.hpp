#ifndef ARPIYI_SCRIPT_MANAGER_HPP
#define ARPIYI_SCRIPT_MANAGER_HPP

#include "asset_manager.hpp"
#include "assets/script.hpp"

namespace arpiyi::script_editor {

void init();
void render(bool* p_show);

Handle<assets::Script> get_startup_script();
void set_startup_script(Handle<assets::Script>);

} // namespace arpiyi::script_editor

#endif // ARPIYI_SCRIPT_MANAGER_HPP
