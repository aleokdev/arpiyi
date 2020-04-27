#ifndef ARPIYI_PROJECT_MANAGER_HPP
#define ARPIYI_PROJECT_MANAGER_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::project_manager {

fs::path get_project_path();
void set_project_path(fs::path const&);

}

#endif // ARPIYI_PROJECT_MANAGER_HPP
