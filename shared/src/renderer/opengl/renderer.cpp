/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
/* clang-format on */

#include "renderer/opengl/impl_types.hpp"

#include "assets/map.hpp"
#include <anton/math/matrix4.hpp>
#include <anton/math/transform.hpp>
#include <anton/math/vector2.hpp>
#include <global_tile_size.hpp>

#include <iostream>

namespace arpiyi::renderer {

Renderer::~Renderer() = default;

Renderer::Renderer(GLFWwindow* _w) : window(_w), p_impl(std::make_unique<impl>()) {
    {
        MeshBuilder builder;
        assets::Sprite spr;
        spr.pieces = {{{{0, 0}, {1, 1}}, {{0, 0}, {1, 1}}}};
        builder.add_sprite(spr, {0, 0, 0}, 0, 0);
        p_impl->quad_mesh = builder.finish();
    }

    p_impl->lit_shader = ShaderHandle::from_file("data/shaded_tile.vert", "data/shaded_tile.frag");
    p_impl->unlit_shader = ShaderHandle::from_file("data/basic_tile.vert", "data/basic_tile.frag");

    p_impl->depth_shader = ShaderHandle::from_file("data/depth.vert", "data/depth.frag");
    p_impl->grid_shader = ShaderHandle::from_file("data/grid.vert", "data/grid.frag");
    p_impl->tile_shader_tile_tex_location =
        glGetUniformLocation(p_impl->lit_shader.p_impl->handle, "tile");
    p_impl->tile_shader_shadow_tex_location =
        glGetUniformLocation(p_impl->lit_shader.p_impl->handle, "shadow");

    TextureHandle tex;
    constexpr u32 default_shadow_res_width = 1024;
    constexpr u32 default_shadow_res_height = 1024;
    tex.init(default_shadow_res_width, default_shadow_res_height, TextureHandle::ColorType::depth,
             TextureHandle::FilteringMethod::point);
    p_impl->shadow_depth_fb = Framebuffer(tex);

    p_impl->window_framebuffer.p_impl->handle = 0;
}

ShaderHandle Renderer::lit_shader() const { return p_impl->lit_shader; }
ShaderHandle Renderer::unlit_shader() const { return p_impl->unlit_shader; }

void Renderer::start_frame() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::finish_frame() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

Framebuffer Renderer::get_window_framebuffer() {
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    TextureHandle virtual_window_tex;
    virtual_window_tex.p_impl->width = display_w;
    virtual_window_tex.p_impl->height = display_h;
    // TODO: Check this. Not sure if default framebuffer has linear filtering or not
    virtual_window_tex.p_impl->filter = TextureHandle::FilteringMethod::linear;
    virtual_window_tex.p_impl->color_type = TextureHandle::ColorType::rgba;
    p_impl->window_framebuffer.p_impl->tex = virtual_window_tex;
    p_impl->window_framebuffer.p_impl->handle = 0;
    return p_impl->window_framebuffer;
}

void Renderer::draw_map(RenderMapContext const& data) {
    assert(data.map.get());
    const auto& map = *data.map.get();

    // TODO: Update map mesh on tile place
    if (data.map != data.p_impl->last_map || data.force_mesh_regeneration) {
        auto& meshes = data.p_impl->meshes;
        for (auto& [_, mesh] : meshes) { mesh.unload(); }
        meshes.clear();
        // u64 is a texture handle.
        std::unordered_map<u64, renderer::MeshBuilder> builders;

        for (const auto& l : map.layers) {
            assert(l.get());
            const auto& layer = *l.get();
            for (int y = 0; y < map.height; ++y) {
                for (int x = 0; x < map.width; ++x) {
                    const auto tile = layer.get_tile({x, y});
                    // TODO: Implement slopes (Set vertical_slope and horizontal_slope)
                    if (tile.exists) {
                        assert(tile.parent.tileset.get());
                        builders[tile.parent.tileset.get()->texture.get_id()].add_sprite(
                            tile.sprite(layer, {x, y}),
                            {static_cast<float>(x), static_cast<float>(y),
                             static_cast<float>(tile.height)},
                            0, 0);
                    }
                }
            }
        }

        for (const auto& [tex_id, builder] : builders) { meshes[tex_id] = builder.finish(); }
        data.p_impl->last_map = data.map;
        data.force_mesh_regeneration = false;
    }

    const auto map_to_fb_tile_pos = [&data, map](aml::Vector2 pos) -> aml::Vector2 {
        return {pos.x -
                    static_cast<float>(data.output_fb.get_size().x) / global_tile_size::get() /
                        data.zoom / 2.f +
                    map.width / 2.f,
                pos.y -
                    static_cast<float>(data.output_fb.get_size().y) / global_tile_size::get() /
                        data.zoom / 2.f +
                    map.height / 2.f};
    };

    aml::Matrix4 t_proj_mat = aml::orthographic_rh(
        0, static_cast<float>(data.output_fb.get_size().x) / global_tile_size::get() / data.zoom, 0,
        static_cast<float>(data.output_fb.get_size().y) / global_tile_size::get() / data.zoom, 10,
        -10);

    const aml::Vector2 cam_tile_pos = map_to_fb_tile_pos({-data.cam_pos.x, -data.cam_pos.y});
    // Camera matrix, in tile units.
    aml::Matrix4 cam_mat = aml::Matrix4::identity;
    cam_mat *= aml::translate({cam_tile_pos.x, cam_tile_pos.y, 0});
    cam_mat = aml::inverse(cam_mat);

    aml::Vector2 cam_min_tile_viewing{aml::max(cam_tile_pos.x, 0.f), aml::max(cam_tile_pos.y, 0.f)};
    aml::Vector2 fb_map_tile_limit{
        static_cast<float>(data.output_fb.get_size().x) / global_tile_size::get() / data.zoom,
        static_cast<float>(data.output_fb.get_size().y) / global_tile_size::get() / data.zoom};
    aml::Vector2 cam_max_tile_viewing = {
        map.width - aml::max(-fb_map_tile_limit.x - cam_tile_pos.x + map.width, 0.f),
        map.height - aml::max(-fb_map_tile_limit.y - cam_tile_pos.y + map.height, 0.f)};
    aml::Vector2 cam_tile_viewing_size{aml::abs(cam_max_tile_viewing.x - cam_min_tile_viewing.x),
                                       aml::abs(cam_max_tile_viewing.y - cam_min_tile_viewing.y)};

    // Depth image rendering
    glBindFramebuffer(GL_FRAMEBUFFER, data.p_impl->raw_depth_fb);
    glViewport(0, 0, data.p_impl->depth_tex.width(), data.p_impl->depth_tex.height());
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    const auto map_to_light_coords = [&data](aml::Vector2 map_pos) -> aml::Vector2 {
        if (map_pos.x == 0 && map_pos.y == 0)
            return {0, 0};

        const float distance = aml::length(map_pos);
        const float angle = data.z_light_rotation + std::asin(map_pos.y / distance);
        const float x = distance * aml::cos(angle);
        const float y = distance * aml::sin(angle) * aml::cos(data.x_light_rotation);

        return {x, y};
    };

    // Apply Z/X rotation to projection so that the entire map is aligned with the texture.
    const int z_rotation_quadrant =
        static_cast<int>(std::fmod(data.z_light_rotation, M_PI * 2.f) / (M_PI / 2.f));
    bool rotation_case = z_rotation_quadrant == 0 || z_rotation_quadrant == 2;
    if (data.z_light_rotation < 0)
        rotation_case = !rotation_case;
    const float most_right_y_pos = rotation_case ? 0 : map.height;
    const float most_left_y_pos = rotation_case ? map.height : 0;
    const float most_top_x_pos = rotation_case ? 0 : map.width;
    const float most_bottom_x_pos = rotation_case ? map.width : 0;

    const float light_proj_right =
        map_to_light_coords({cam_max_tile_viewing.x, most_right_y_pos}).x;
    const float light_proj_left = map_to_light_coords({cam_min_tile_viewing.x, most_left_y_pos}).x;
    float light_proj_top = map_to_light_coords({most_top_x_pos, cam_min_tile_viewing.y}).y;
    float light_proj_bottom = map_to_light_coords({most_bottom_x_pos, cam_max_tile_viewing.y}).y;
    aml::Matrix4 lightProjection = aml::orthographic_rh(
        light_proj_left, light_proj_right, light_proj_bottom, light_proj_top, -20.0f, 20.0f);

    // To create the light view, we position the light as if it were a camera and then invert the
    // matrix.
    aml::Matrix4 lightView = aml::Matrix4::identity;
    // We want the light to be rotated on the Z and X axis to make it seem there's some
    // directionality to it.
    lightView *= aml::rotate_z(data.z_light_rotation) * aml::rotate_x(data.x_light_rotation);
    lightView = aml::inverse(lightView);
    aml::Matrix4 lightSpaceMatrix = lightProjection * lightView;
    glUseProgram(p_impl->depth_shader.p_impl->handle);
    glUniformMatrix4fv(3, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
    draw_meshes(data.p_impl->meshes);

    // Regular image rendering
    glBindFramebuffer(GL_FRAMEBUFFER, data.output_fb.p_impl->handle);
    glViewport(0, 0, data.output_fb.get_size().x, data.output_fb.get_size().y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(p_impl->lit_shader.p_impl->handle);
    glUniform1i(p_impl->tile_shader_tile_tex_location, 0); // Set tile sampler2D to GL_TEXTURE0
    glUniform1i(p_impl->tile_shader_shadow_tex_location,
                1);                                           // Set shadow sampler2D to GL_TEXTURE1
    glUniformMatrix4fv(1, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
    glUniformMatrix4fv(2, 1, GL_FALSE, cam_mat.get_raw());    // View matrix
    glUniformMatrix4fv(3, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.p_impl->handle);
    draw_meshes(data.p_impl->meshes);

    if (data.draw_grid) {
        // Draw mesh grid
        glUseProgram(p_impl->grid_shader.p_impl->handle);
        glBindVertexArray(p_impl->quad_mesh.p_impl->vao);
        glUniform4f(3, .9f, .9f, .9f, .4f); // Grid color
        glUniform2ui(4, map.width, map.height);

        aml::Matrix4 model = aml::Matrix4::identity;
        model *= aml::scale({static_cast<float>(map.width), static_cast<float>(map.height), 0});
        glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw());      // Model matrix
        glUniformMatrix4fv(1, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
        glUniformMatrix4fv(2, 1, GL_FALSE, cam_mat.get_raw());    // View matrix

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);
    }
    if (data.draw_entities) {
        glUseProgram(p_impl->lit_shader.p_impl->handle);
        glUniform1i(p_impl->tile_shader_tile_tex_location,
                    0); // Set tile sampler2D to GL_TEXTURE0
        glUniform1i(p_impl->tile_shader_shadow_tex_location,
                    1); // Set shadow sampler2D to GL_TEXTURE1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.p_impl->handle);
        glBindVertexArray(p_impl->quad_mesh.p_impl->vao);
        glUniformMatrix4fv(1, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
        glUniformMatrix4fv(2, 1, GL_FALSE, cam_mat.get_raw());    // View matrix
        glUniformMatrix4fv(3, 1, GL_FALSE,
                           lightSpaceMatrix.get_raw()); // Light space matrix
        glActiveTexture(GL_TEXTURE0);

        for (const auto& entity_handle : map.entities) {
            assert(entity_handle.get());
            if (auto entity = entity_handle.get()) {
                if (auto sprite = entity->sprite.get()) {
                    assert(sprite->texture.get());
                    glBindTexture(GL_TEXTURE_2D, sprite->texture.get()->handle.p_impl->handle);

                    // TODO: Cache sprite meshes
                    MeshHandle mesh;
                    MeshBuilder builder;
                    builder.add_sprite(*sprite, aml::Vector3(entity->pos, 0.1f), 0, 0);

                    aml::Matrix4 model = aml::Matrix4::identity;

                    // Translate by position of entity. Pivot is bound to the mesh so we don't need
                    // to worry about it
                    model *= aml::translate(aml::Vector3(entity->pos));

                    glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw()); // Model matrix
                    glBindVertexArray(mesh.p_impl->vao);

                    glDrawArrays(GL_TRIANGLES, 0, mesh.p_impl->vertex_count);
                    mesh.unload();
                }
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::draw_tileset(RenderTilesetContext const& data) {
    assert(data.tileset.get());
    const auto& tileset = *data.tileset.get();
    if (data.tileset != data.p_impl->last_tileset) {
        MeshBuilder builder;
        const auto tileset_size = tileset.size_in_tile_units();
        for (u64 tile_i = 0; tile_i < tileset.tile_count(); ++tile_i) {
            builder.add_sprite(assets::Tileset::Tile{data.tileset, tile_i}.preview_sprite(),
                               {static_cast<float>(tile_i % tileset_size.x),
                                -static_cast<float>(tile_i / tileset_size.x) - 1.f, 0},
                               0, 0);
        }
        data.p_impl->tileset_mesh.unload();
        data.p_impl->tileset_mesh = builder.finish();
        data.p_impl->last_tileset = data.tileset;
    }

    aml::Matrix4 t_proj_mat = aml::orthographic_rh(
        0, static_cast<float>(data.output_fb.get_size().x) / global_tile_size::get() / data.zoom,
        -static_cast<float>(data.output_fb.get_size().y) / global_tile_size::get() / data.zoom, 0,
        10, -10);

    // Camera matrix, in tile units.
    aml::Matrix4 cam_mat = aml::translate({data.cam_pos.x, data.cam_pos.y, 0});
    cam_mat = aml::inverse(cam_mat);

    // Regular image rendering
    glBindFramebuffer(GL_FRAMEBUFFER, data.output_fb.p_impl->handle);
    glViewport(0, 0, data.output_fb.get_size().x, data.output_fb.get_size().y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(p_impl->unlit_shader.p_impl->handle);
    aml::Matrix4 model = aml::Matrix4::identity;
    glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw());      // Model matrix
    glUniformMatrix4fv(1, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
    glUniformMatrix4fv(2, 1, GL_FALSE, cam_mat.get_raw());    // View matrix
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tileset.texture.get()->handle.p_impl->handle);
    glBindVertexArray(data.p_impl->tileset_mesh.p_impl->vao);
    glDrawArrays(GL_TRIANGLES, 0, data.p_impl->tileset_mesh.p_impl->vertex_count);

    if (data.draw_grid) {
        // Draw mesh grid
        glUseProgram(p_impl->grid_shader.p_impl->handle);
        glBindVertexArray(p_impl->quad_mesh.p_impl->vao);
        glUniform4f(3, .9f, .9f, .9f, .4f); // Grid color
        const math::IVec2D tileset_size_tile_units = tileset.size_in_tile_units();
        const math::IVec2D tileset_size_actual_tile_size = {
            tileset_size_tile_units.x,
            static_cast<i32>(tileset.tile_count() / tileset_size_tile_units.x)};
        glUniform2ui(4, tileset_size_actual_tile_size.x, tileset_size_actual_tile_size.y);

        aml::Matrix4 model = aml::Matrix4::identity;
        model *= aml::scale({static_cast<float>(tileset_size_actual_tile_size.x),
                             -static_cast<float>(tileset_size_actual_tile_size.y), 0});
        glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw());
        glUniformMatrix4fv(1, 1, GL_FALSE, t_proj_mat.get_raw());
        glUniformMatrix4fv(2, 1, GL_FALSE, cam_mat.get_raw());

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::draw_meshes(std::unordered_map<u64, MeshHandle> const& batches) {
    aml::Matrix4 model = aml::Matrix4::identity;

    glActiveTexture(GL_TEXTURE0);
    // Draw each mesh
    for (auto& [texture_id, mesh] : batches) {
        assert(Handle<assets::TextureAsset>(texture_id).get());
        glBindVertexArray(mesh.p_impl->vao);
        glBindTexture(GL_TEXTURE_2D,
                      Handle<assets::TextureAsset>(texture_id).get()->handle.p_impl->handle);

        glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw());

        glDrawArrays(GL_TRIANGLES, 0, mesh.p_impl->vertex_count);
    }
}

void Renderer::draw(DrawCmdList const& draw_commands, Framebuffer const& output_fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, p_impl->shadow_depth_fb.p_impl->handle);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
    glViewport(0, 0, p_impl->shadow_depth_fb.texture().width(),
               p_impl->shadow_depth_fb.texture().height());

    aml::Vector2 camera_view_size_in_tiles{
        output_fb.texture().width() / global_tile_size::get() / draw_commands.camera.zoom,
        output_fb.texture().height() / global_tile_size::get() / draw_commands.camera.zoom};
    aml::Matrix4 view =
        aml::inverse(aml::translate(draw_commands.camera.position));
    aml::Matrix4 proj;
    if(draw_commands.camera.center_view) {
        proj = aml::orthographic_rh(
            -camera_view_size_in_tiles.x / 2.f, camera_view_size_in_tiles.x / 2.f,
            -camera_view_size_in_tiles.y / 2.f, camera_view_size_in_tiles.y / 2.f, -20.0f, 20.0f);
    } else {
        proj = aml::orthographic_rh(
            0, camera_view_size_in_tiles.x,
            -camera_view_size_in_tiles.y, 0, -20.0f, 20.0f);
    }
    // To create the light view, we position the light as if it were a camera and then
    // invert the matrix.
    aml::Matrix4 lightView = aml::translate(draw_commands.camera.position);
    // We want the light to be rotated on the Z and X axis to make it seem there's some
    // directionality to it.
    lightView *= aml::rotate_z(-M_PI / 5.f) * aml::rotate_x(-M_PI / 5.f) * aml::scale(2.f);
    lightView = aml::inverse(lightView);
    aml::Matrix4 lightSpaceMatrix = proj * lightView;

    glUseProgram(p_impl->depth_shader.p_impl->handle);
    for (const auto& cmd : draw_commands.commands) {
        if (!cmd.cast_shadows)
            continue;
        aml::Matrix4 model = aml::translate(cmd.transform.position);

        glBindVertexArray(cmd.mesh.p_impl->vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmd.texture.p_impl->handle);
        glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw()); // Model matrix
        glUniformMatrix4fv(3, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
        glDrawArrays(GL_TRIANGLES, 0, cmd.mesh.p_impl->vertex_count);
    }
    glViewport(0, 0, output_fb.texture().width(), output_fb.texture().height());
    glBindFramebuffer(GL_FRAMEBUFFER, output_fb.p_impl->handle);
    for (const auto& cmd : draw_commands.commands) {
        bool is_lit = cmd.shader.p_impl->shadow_tex_location != static_cast<u32>(-1);
        aml::Matrix4 model = aml::translate(cmd.transform.position);

        glUseProgram(cmd.shader.p_impl->handle);
        glUniformMatrix4fv(0, 1, GL_FALSE, model.get_raw()); // Model matrix
        glUniformMatrix4fv(1, 1, GL_FALSE, proj.get_raw());  // Projection matrix
        glUniformMatrix4fv(2, 1, GL_FALSE, view.get_raw());  // View matrix
        if(is_lit)
            glUniformMatrix4fv(3, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
        glBindVertexArray(cmd.mesh.p_impl->vao);

        glUniform1i(cmd.shader.p_impl->tile_tex_location, 0); // Set tile sampler2D to GL_TEXTURE0
        glUniform1i(cmd.shader.p_impl->shadow_tex_location,
                    1); // Set shadow sampler2D to GL_TEXTURE1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cmd.texture.p_impl->handle);
        if(is_lit) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, p_impl->shadow_depth_fb.texture().p_impl->handle);
        }
        glUseProgram(cmd.shader.p_impl->handle);

        // Light space matrix is already set if used
        glDrawArrays(GL_TRIANGLES, 0, cmd.mesh.p_impl->vertex_count);
    }
}

void Renderer::clear(Framebuffer& fb, aml::Vector4 color) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb.p_impl->handle);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace arpiyi::renderer