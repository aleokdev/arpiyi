#include "systems/editor_renderer.hpp"

namespace arpiyi_editor::editor::renderer {

std::unordered_map<std::size_t, Toolbar> toolbars;
std::size_t last_toolbar_id = 0;

Toolbar& add_toolbar(std::string const& name, std::function<void(Toolbar&)> const& callback) {
    const auto id_to_use = last_toolbar_id++;
    return toolbars.emplace(id_to_use, Toolbar{id_to_use, name, callback}).first->second;
}

void remove_toolbar(std::size_t id) { toolbars.erase(id); }

bool has_toolbar(std::size_t id) { return toolbars.find(id) != toolbars.end(); }

std::unordered_map<std::size_t, Window> windows;
std::size_t last_window_id = 0;

Window& add_window(std::string const& title) {
    const auto id_to_use = last_window_id++;
    return windows.emplace(id_to_use, Window{id_to_use, title}).first->second;
}

void remove_window(std::size_t id) { windows.erase(id); }

bool has_window(std::size_t id) { return windows.find(id) != windows.end(); }

void render() {}

} // namespace arpiyi_editor::editor::renderer
