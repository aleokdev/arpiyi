#include "editor/editor_lua_wrapper.hpp"
#include "plugin_manager.hpp"

namespace arpiyi_editor::editor::lua_wrapper {

void init() {
    /* clang-format off */
    plugin_manager::get_state().new_usertype<ToolbarWrapper>("EditorToolbar",
        sol::constructors<void(std::string const&, sol::function)>(),
        "delete", &ToolbarWrapper::lua_destroy);
    plugin_manager::get_state().new_usertype<WindowWrapper>("EditorWindow",
         sol::constructors<void(std::string const&)>(),
         "delete", &WindowWrapper::lua_destroy);
    /* clang-format on */
}

ToolbarWrapper::ToolbarWrapper(std::string const& name, sol::function callback) {
    using Toolbar = editor::renderer::Toolbar;
    auto it = editor::renderer::add_toolbar(name, [callback](Toolbar& t) { callback(ToolbarWrapper(t.id)); });
    id = it.id;
}

void ToolbarWrapper::lua_destroy() {
    editor::renderer::remove_toolbar(id);
}

WindowWrapper::WindowWrapper(std::string const& title) {
    using Window = editor::renderer::Window;
    auto it = editor::renderer::add_window(title);
    id = it.id;
}

void WindowWrapper::lua_destroy() {
    editor::renderer::remove_window(id);
}

} // namespace arpiyi_editor::editor::lua_wrapper