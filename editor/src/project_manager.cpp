#include "project_manager.hpp"

namespace arpiyi::project_manager {

fs::path current_project_path;

fs::path get_project_path() {
    return current_project_path;
}

void set_project_path(fs::path const& path) {
    current_project_path = path;
}

}