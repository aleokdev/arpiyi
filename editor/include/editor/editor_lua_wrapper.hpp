#ifndef ARPIYI_EDITOR_LUA_WRAPPER_HPP
#define ARPIYI_EDITOR_LUA_WRAPPER_HPP

#include "editor_renderer.hpp"
#include <sol/sol.hpp>

namespace arpiyi_editor::editor::lua_wrapper {

void init();

struct ToolbarWrapper {
    ToolbarWrapper(std::string const& name, sol::function callback);
    void lua_destroy();

    std::size_t id;
};

struct WindowWrapper {
    WindowWrapper(std::string const& title);
    void lua_destroy();

    std::size_t id;
};

}; // namespace arpiyi_editor::systems::lua_wrapper

#endif // ARPIYI_EDITOR_LUA_WRAPPER_HPP
