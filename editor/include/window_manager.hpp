#ifndef ARPIYI_WINDOW_MANAGER_HPP
#define ARPIYI_WINDOW_MANAGER_HPP

#include <anton/math/matrix4.hpp>

#include "util/math.hpp"
#include "renderer/renderer.hpp"

namespace aml = anton::math;

struct GLFWwindow;

namespace arpiyi::window_manager {

bool init();

renderer::Renderer& get_renderer();
GLFWwindow* get_window();
aml::Matrix4 get_projection();
math::IVec2D get_framebuf_size();

} // namespace arpiyi::window_manager

#endif // ARPIYI_WINDOW_MANAGER_HPP
