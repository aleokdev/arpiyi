#ifndef ARPIYI_WINDOW_MANAGER_HPP
#define ARPIYI_WINDOW_MANAGER_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

namespace arpiyi_editor::window_manager {

void init();

GLFWwindow* get_window();

}

#endif // ARPIYI_WINDOW_MANAGER_HPP
