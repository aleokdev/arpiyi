#ifndef ARPIYI_EDITOR_LUA_WRAPPER_HPP
#define ARPIYI_EDITOR_LUA_WRAPPER_HPP

#include "editor_renderer.hpp"
#include <sol/sol.hpp>

namespace arpiyi::editor::lua_wrapper {

void init();

struct ToolbarWrapper {
    ToolbarWrapper(std::string const& name, sol::function callback);
    ToolbarWrapper(std::size_t id) : id(id){};
    void lua_destroy();

    std::size_t id;
};

struct WindowWrapper {
    WindowWrapper(std::string const& title);
    WindowWrapper(std::size_t id) : id(id){};
    void lua_destroy();

    std::size_t id;
};

}; // namespace arpiyi::editor::lua_wrapper

#endif // ARPIYI_EDITOR_LUA_WRAPPER_HPP
