#ifndef ARPIYI_WINDOW_MANAGER_HPP
#define ARPIYI_WINDOW_MANAGER_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <glm/mat4x4.hpp>

#include "util/math.hpp"

namespace arpiyi_editor::window_manager {

bool init();

GLFWwindow* get_window();
glm::mat4 get_projection();
math::IVec2D get_framebuf_size();

}

#endif // ARPIYI_WINDOW_MANAGER_HPP
