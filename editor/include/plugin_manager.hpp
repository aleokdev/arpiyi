#ifndef ARPIYI_PLUGIN_MANAGER_HPP
#define ARPIYI_PLUGIN_MANAGER_HPP

#include <sol/sol.hpp>

#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::plugin_manager {

void init();

void load_plugins(fs::path const& dir);

void update();

sol::state& get_state();

} // namespace arpiyi::plugin_manager

#endif // ARPIYI_PLUGIN_MANAGER_HPP
