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

static arpiyi::Handle<arpiyi::assets::Shader> sprite_shader;
static aml::Matrix4 proj_mat;

namespace arpiyi::default_api_impls {

void init() {
    sprite_shader = asset_manager::load<assets::Shader>({"data/basic.vert", "data/basic.frag"});
}

} // namespace arpiyi::default_api_impls

namespace arpiyi::api {

void render_map_layer(assets::Map const& map, assets::Map::Layer& layer) {
    if (!layer.get_mesh().get()) {
        layer.regenerate_mesh();
    }
    assert(layer.get_mesh().get());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, layer.tileset.get()->texture.get()->handle);

    glUseProgram(sprite_shader.get()->handle);
    glBindVertexArray(layer.get_mesh().get()->vao);

    const auto& cam = game_data_manager::get_game_data().cam;

    float map_total_width = map.width * global_tile_size::get() * cam->zoom;
    float map_total_height = map.height * global_tile_size::get() * cam->zoom;

    aml::Matrix4 model = aml::Matrix4::identity;

    // Center map
    model *= aml::translate({-map_total_width / 2.f, map_total_height / 2.f, 0});
    // Translate by camera vector
    model *= aml::translate(aml::Vector3(cam->pos));
    // Scale accordingly
    model *= aml::scale(aml::Vector3{map_total_width, -map_total_height, 1});

    glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
    glUniformMatrix4fv(2, 1, GL_FALSE, proj_mat.get_raw());

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, map.width * map.height * quad_verts);
}

void map_screen_layer_render_cb() {
    aml::Vector2 output_size = window_manager::get_framebuf_size();
    proj_mat = aml::orthographic_rh(-output_size.x / 2.f, output_size.x / 2.f, output_size.y / 2.f,
                                    -output_size.y / 2.f, -10000.f, 10000.f);

    if (auto map = game_data_manager::get_game_data().current_map.get()) {
        for (auto& layer : map->layers) {
            assert(layer.get());
            render_map_layer(*map, *layer.get());
        }
    }
}

KeyState get_key_state(InputKey key) {
    // TODO: Implement just_pressed and just_released
    return {glfwGetKey(window_manager::get_window(), static_cast<int>(key)) == GLFW_PRESS, false, false};
}

} // namespace arpiyi::api