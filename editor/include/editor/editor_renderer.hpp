#ifndef ARPIYI_EDITOR_RENDERER_HPP
#define ARPIYI_EDITOR_RENDERER_HPP

#include <functional>

namespace arpiyi::editor::renderer {

struct Toolbar;
struct Toolbar {
    std::size_t id;
    std::string name;
    std::function<void(Toolbar&)> callback;
};

Toolbar& add_toolbar(std::string const& name, std::function<void(Toolbar&)> const& callback);
void remove_toolbar(std::size_t id);
bool has_toolbar(std::size_t id);

struct Window {
    std::size_t id;
    std::string title;
};

Window& add_window(std::string const& title);
void remove_window(std::size_t id);
bool has_window(std::size_t id);

void render();

} // namespace arpiyi::editor::renderer

#endif // ARPIYI_EDITOR_RENDERER_HPP
