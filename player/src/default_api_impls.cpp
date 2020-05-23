#include <api/api.hpp>

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <iostream>
#include "assets/map.hpp"
#include "asset_manager.hpp"
#include "game_data_manager.hpp"
#include "window_manager.hpp"
#include "global_tile_size.hpp"

#include <anton/math/matrix4.hpp>
#include <anton/math/transform.hpp>

namespace aml = anton::math;

namespace arpiyi::default_api_impls {

void init() {
}

} // namespace arpiyi::default_api_impls

namespace arpiyi::api {
using namespace default_api_impls;

void map_screen_layer_render_cb() {
    if (auto m = game_data_manager::get_game_data().current_map.get()) {
        renderer::DrawCmdList cmd_list;
        cmd_list.camera = renderer::Camera{aml::Vector3(game_data_manager::get_game_data().cam->pos), game_data_manager::get_game_data().cam->zoom};
        m->draw_to_cmd_list(window_manager::get_renderer(), cmd_list);
        window_manager::get_renderer().draw(cmd_list, window_manager::get_renderer().get_window_framebuffer());
    }
}

KeyState get_key_state(InputKey key) {
    // TODO: Implement just_pressed and just_released
    return {glfwGetKey(window_manager::get_window(), static_cast<int>(key)) == GLFW_PRESS, false,
            false};
}

} // namespace arpiyi::api