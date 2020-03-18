#include "systems/editor_lua_wrapper.hpp"
#include "editor.hpp"

namespace arpiyi_editor::editor::lua_wrapper {

ToolbarWrapper ToolbarWrapper::create(std::string const& name, sol::function callback) {
    using Toolbar = editor::renderer::Toolbar;
    auto it = editor::renderer::add_toolbar(name, [callback](Toolbar& t) { callback(t); });
    return ToolbarWrapper{it.id};
}

void ToolbarWrapper::lua_destroy() {}

WindowWrapper WindowWrapper::create(std::string const& title) {
    using Window = editor::renderer::Window;
    auto it = editor::renderer::add_window(title);
    return WindowWrapper{it.id};
}

void WindowWrapper::lua_destroy() {}

} // namespace arpiyi_editor::editor::lua_wrapper