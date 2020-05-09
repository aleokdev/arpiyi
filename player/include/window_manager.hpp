#ifndef ARPIYI_WINDOW_MANAGER_HPP
#define ARPIYI_WINDOW_MANAGER_HPP

#include <anton/math/matrix4.hpp>
#include "util/math.hpp"
#include "renderer/renderer.hpp"

struct GLFWwindow;
namespace aml = anton::math;

namespace arpiyi::window_manager {

bool init();
renderer::Renderer& get_renderer();
GLFWwindow* get_window();
aml::Matrix4 get_projection();
math::IVec2D get_framebuf_size();

}

#endif // ARPIYI_WINDOW_MANAGER_HPP
