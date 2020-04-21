#include "plugin_manager.hpp"

#include <sol/sol.hpp>

#include <iostream>
#include <vector>

std::vector<sol::coroutine> coroutines;
sol::state lua{};

namespace arpiyi::plugin_manager {

void init() {
    lua.open_libraries(sol::lib::base, sol::lib::os, sol::lib::io, sol::lib::table, sol::lib::math,
                       sol::lib::debug, sol::lib::bit32, sol::lib::coroutine, sol::lib::string);
}
void load_plugins(fs::path const& dir) {
    for (auto& file : fs::directory_iterator(dir)) {
        sol::thread thread(sol::thread::create(lua));

        sol::load_result result(lua.load_file(file.path().generic_string()));

        if (!result.valid() || result.status() != sol::load_status::ok) {
            std::cerr << "Could not load plugin " << file.path().filename() << ":" << std::endl;
            sol::script_throw_on_error(lua.lua_state(), result);
        }

        sol::function func_nothread = result;
        sol::function func(thread.thread_state(), func_nothread);
        func(); // Call it for the first time to make the script initialize whatever it needs
        coroutines.emplace_back(func);
    }
}

void update() {
    for (auto& coro : coroutines) coro();
    lua.collect_garbage();
}

sol::state& get_state() { return lua; }

} // namespace arpiyi::plugin_manager