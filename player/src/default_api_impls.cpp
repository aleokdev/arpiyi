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
static arpiyi::Handle<arpiyi::assets::Shader> tile_shader;
static arpiyi::Handle<arpiyi::assets::Mesh> quad_mesh;
static aml::Matrix4 proj_mat;

namespace arpiyi::default_api_impls {

void init() {
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());
    sprite_shader = asset_manager::load<assets::Shader>({"data/basic.vert", "data/basic.frag"});
    tile_shader = asset_manager::load<assets::Shader>({"data/basic.vert", "data/tile_uv.frag"});
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

    const float map_total_width = map.width * global_tile_size::get() * cam->zoom;
    const float map_total_height = map.height * global_tile_size::get() * cam->zoom;

    aml::Matrix4 model = aml::Matrix4::identity;

    // Move map downwards
    model *= aml::translate({0, map_total_height, 0});
    // Translate by camera vector
    model *= aml::translate(aml::Vector3(-cam->pos.x, -cam->pos.y, 0) * global_tile_size::get() * cam->zoom);
    // Scale accordingly
    model *= aml::scale(aml::Vector3{map_total_width, -map_total_height, 1});

    glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
    glUniformMatrix4fv(2, 1, GL_FALSE, proj_mat.get_raw());

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, map.width * map.height * quad_verts);
}

void render_map_entities(assets::Map const& map) {
    const auto& cam = game_data_manager::get_game_data().cam;

    const float map_total_width = map.width * global_tile_size::get() * cam->zoom;
    const float map_total_height = map.height * global_tile_size::get() * cam->zoom;

    for (const auto& entity_handle : map.entities) {
        assert(entity_handle.get());
        if (auto entity = entity_handle.get()) {
            if (auto sprite = entity->sprite.get()) {
                assert(sprite->texture.get());
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sprite->texture.get()->handle);

                glUseProgram(tile_shader.get()->handle);
                glBindVertexArray(quad_mesh.get()->vao);

                const auto size_in_pixels = sprite->get_size_in_pixels();
                float sprite_total_width = size_in_pixels.x * cam->zoom;
                float sprite_total_height = size_in_pixels.y * cam->zoom;

                aml::Matrix4 model = aml::Matrix4::identity;

                // Apply sprite pivot
                model *= aml::translate(aml::Vector3(-sprite->pivot) /
                                        aml::Vector3(map_total_width, map_total_height, 1));
                // Translate by position of entity
                model *=
                    aml::translate(aml::Vector3(entity->pos) * global_tile_size::get() * cam->zoom);
                // Translate by camera vector
                model *=
                    aml::translate(aml::Vector3(-cam->pos) * global_tile_size::get() * cam->zoom);
                // Scale accordingly
                model *= aml::scale(aml::Vector3{sprite_total_width, sprite_total_height, 1});

                glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
                glUniformMatrix4fv(2, 1, GL_FALSE, proj_mat.get_raw());
                glUniform2f(3, sprite->uv_min.x, sprite->uv_min.y);
                glUniform2f(4, sprite->uv_max.x, sprite->uv_max.y);

                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0, quad_verts);
            }
        }
    }
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
        render_map_entities(*map);
    }
}

KeyState get_key_state(InputKey key) {
    // TODO: Implement just_pressed and just_released
    return {glfwGetKey(window_manager::get_window(), static_cast<int>(key)) == GLFW_PRESS, false,
            false};
}

} // namespace arpiyi::api