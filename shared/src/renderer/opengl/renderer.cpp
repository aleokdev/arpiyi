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
    p_impl->lit_shader = ShaderHandle::from_file("data/shaded_tile.vert", "data/shaded_tile.frag");
    p_impl->unlit_shader = ShaderHandle::from_file("data/basic_tile.vert", "data/basic_tile.frag");

    p_impl->depth_shader = ShaderHandle::from_file("data/depth.vert", "data/depth.frag");

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

void Renderer::draw(DrawCmdList const& draw_commands, Framebuffer const& output_fb) {
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

    glBindFramebuffer(GL_FRAMEBUFFER, p_impl->shadow_depth_fb.p_impl->handle);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
    glViewport(0, 0, p_impl->shadow_depth_fb.texture().width(),
               p_impl->shadow_depth_fb.texture().height());
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
    glClear(GL_DEPTH_BUFFER_BIT);
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