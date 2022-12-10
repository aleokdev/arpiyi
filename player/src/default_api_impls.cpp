#include <api/api.hpp>

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <iostream>
#include "assets/map.hpp"
#include "assets/shader.hpp"
#include "asset_manager.hpp"
#include "game_data_manager.hpp"
#include "window_manager.hpp"
#include "global_tile_size.hpp"

#include <anton/math/matrix4.hpp>
#include <anton/math/transform.hpp>

namespace aml = anton::math;

namespace arpiyi::default_api_impls {

static std::unique_ptr<renderer::RenderMapContext> render_map_context;

void init() {
    render_map_context = std::make_unique<renderer::RenderMapContext>(window_manager::get_framebuf_size());
}

} // namespace arpiyi::default_api_impls

namespace arpiyi::api {
using namespace default_api_impls;

void map_screen_layer_render_cb() {
    render_map_context->map = game_data_manager::get_game_data().current_map;
    render_map_context->cam_pos = game_data_manager::get_game_data().cam->pos;
    render_map_context->zoom = game_data_manager::get_game_data().cam->zoom;
    render_map_context->output_fb = window_manager::get_renderer().get_window_framebuffer();
    window_manager::get_renderer().draw_map(*render_map_context);
}

KeyState get_key_state(InputKey key) {
    // TODO: Implement just_pressed and just_released
    return {glfwGetKey(window_manager::get_window(), static_cast<int>(key)) == GLFW_PRESS, false,
            false};
}

} // namespace arpiyi::api