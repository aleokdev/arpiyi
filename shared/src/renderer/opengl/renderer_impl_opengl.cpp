/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
/* clang-format on */

#include "renderer/renderer.hpp"

#include "assets/map.hpp"
#include <anton/math/matrix4.hpp>
#include <anton/math/transform.hpp>
#include <anton/math/vector2.hpp>
#include <assets/shader.hpp>
#include <global_tile_size.hpp>

namespace arpiyi::renderer {

struct Renderer::impl {
    Handle<assets::Mesh> quad_mesh;
    Handle<assets::Shader> tile_shader;
    Handle<assets::Shader> entity_shader;
    Handle<assets::Shader> depth_shader;
    Handle<assets::Shader> grid_shader;
    unsigned int tile_shader_tile_tex_location;
    unsigned int tile_shader_shadow_tex_location;
    unsigned int entity_shader_tile_tex_location;
    unsigned int entity_shader_shadow_tex_location;
};

Renderer::~Renderer() = default;

Renderer::Renderer(GLFWwindow* _w) : window(_w), p_impl(std::make_unique<impl>()) {
    p_impl->quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    p_impl->tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    p_impl->depth_shader =
        asset_manager::load<assets::Shader>({"data/depth.vert", "data/empty.frag"});
    p_impl->entity_shader =
        asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile_uv.frag"});
    p_impl->grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    p_impl->tile_shader_tile_tex_location =
        glGetUniformLocation(p_impl->tile_shader.get()->handle, "tile");
    p_impl->tile_shader_shadow_tex_location =
        glGetUniformLocation(p_impl->tile_shader.get()->handle, "shadow");
    p_impl->entity_shader_tile_tex_location =
        glGetUniformLocation(p_impl->entity_shader.get()->handle, "tile");
    p_impl->entity_shader_shadow_tex_location =
        glGetUniformLocation(p_impl->entity_shader.get()->handle, "shadow");
}

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

struct Framebuffer::impl {
    unsigned int handle;
    assets::Texture tex;
};

Framebuffer::Framebuffer() : p_impl(std::make_unique<impl>()) {
    p_impl->handle = static_cast<unsigned int>(-1);
}

Framebuffer::Framebuffer(math::IVec2D size) : p_impl(std::make_unique<impl>()) { set_size(size); }

void Framebuffer::set_size(math::IVec2D size) {
    auto& handle = p_impl->handle;
    auto& tex = p_impl->tex;
    if (tex.handle != assets::Texture::nohandle)
        glDeleteTextures(1, &tex.handle);
    if (handle != assets::Texture::nohandle)
        glDeleteFramebuffers(1, &handle);
    glGenTextures(1, &tex.handle);
    glBindTexture(GL_TEXTURE_2D, tex.handle);
    glCreateFramebuffers(1, &handle);

    tex.w = size.x;
    tex.h = size.y;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.handle, 0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

math::IVec2D Framebuffer::get_size() const {
    return {static_cast<i32>(p_impl->tex.w), static_cast<i32>(p_impl->tex.h)};
}

ImTextureID Framebuffer::get_imgui_id() const {
    return reinterpret_cast<ImTextureID>(p_impl->tex.handle);
}

Framebuffer::~Framebuffer() {
    if (glfwGetCurrentContext() == nullptr)
        return;
    if (p_impl->tex.handle == assets::Texture::nohandle)
        glDeleteTextures(1, &p_impl->tex.handle);
    if (p_impl->handle != static_cast<unsigned int>(-1))
        glDeleteFramebuffers(1, &p_impl->handle);
}

struct RenderMapContext::impl {
    unsigned int raw_depth_fb;
    assets::Texture depth_tex;
};

RenderMapContext::RenderMapContext(math::IVec2D shadow_resolution) :
    p_impl(std::make_unique<impl>()) {
    set_shadow_resolution(shadow_resolution);
}
RenderMapContext::~RenderMapContext() {
    // glDeleteTextures(1, &p_impl->depth_tex.handle);
    // glDeleteFramebuffers(1, &p_impl->raw_depth_fb);
}

namespace detail {
static void draw_map(assets::Map const& map);
}

void RenderMapContext::set_shadow_resolution(math::IVec2D size) {
    auto& shadow_fb_handle = p_impl->raw_depth_fb;
    auto& shadow_tex = p_impl->depth_tex;
    if (shadow_tex.handle != assets::Texture::nohandle)
        glDeleteTextures(1, &shadow_tex.handle);
    glGenTextures(1, &shadow_tex.handle);
    glBindTexture(GL_TEXTURE_2D, shadow_tex.handle);
    if (shadow_fb_handle != static_cast<unsigned int>(-1))
        glDeleteFramebuffers(1, &shadow_fb_handle);
    glCreateFramebuffers(1, &shadow_fb_handle);

    shadow_tex.w = size.x; // Shadowmap width
    shadow_tex.h = size.y; // Shadowmap height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb_handle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_tex.handle,
                           0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}
math::IVec2D RenderMapContext::get_shadow_resolution() const {
    return {static_cast<i32>(p_impl->depth_tex.w), static_cast<i32>(p_impl->depth_tex.h)};
}

void Renderer::draw_map(RenderMapContext const& data) {
    assert(data.map.get());
    const auto& map = *data.map.get();
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
    glViewport(0, 0, data.p_impl->depth_tex.w, data.p_impl->depth_tex.h);
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
    glUseProgram(p_impl->depth_shader.get()->handle);
    glUniformMatrix4fv(2, 1, GL_FALSE, lightSpaceMatrix.get_raw());
    detail::draw_map(map);

    // Regular image rendering
    glBindFramebuffer(GL_FRAMEBUFFER, data.output_fb.p_impl->handle);
    glViewport(0, 0, data.output_fb.get_size().x, data.output_fb.get_size().y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(p_impl->tile_shader.get()->handle);
    glUniform1i(p_impl->tile_shader_tile_tex_location, 0); // Set tile sampler2D to GL_TEXTURE0
    glUniform1i(p_impl->tile_shader_shadow_tex_location,
                1);                                           // Set shadow sampler2D to GL_TEXTURE1
    glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
    glUniformMatrix4fv(4, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
    glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw());          // View matrix
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.handle);
    detail::draw_map(map);

    if (data.draw_grid) {
        // Draw mesh grid
        glUseProgram(p_impl->grid_shader.get()->handle);
        glBindVertexArray(p_impl->quad_mesh.get()->vao);
        glUniform4f(3, .9f, .9f, .9f, .4f); // Grid color
        glUniform2ui(4, map.width, map.height);

        aml::Matrix4 model = aml::Matrix4::identity;
        model *= aml::scale({static_cast<float>(map.width), static_cast<float>(map.height), 0});
        glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
        glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw());
        glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw());

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);
    }
    if (data.draw_entities) {
        glUseProgram(p_impl->entity_shader.get()->handle);
        glUniform1i(p_impl->entity_shader_tile_tex_location,
                    0); // Set tile sampler2D to GL_TEXTURE0
        glUniform1i(p_impl->entity_shader_shadow_tex_location,
                    1); // Set shadow sampler2D to GL_TEXTURE1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.handle);
        for (const auto& entity_handle : map.entities) {
            assert(entity_handle.get());
            if (auto entity = entity_handle.get()) {
                if (auto sprite = entity->sprite.get()) {
                    assert(sprite->texture.get());
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, sprite->texture.get()->handle);

                    glBindVertexArray(p_impl->quad_mesh.get()->vao);

                    const auto size_in_pixels = sprite->get_size_in_pixels();
                    const auto size_in_tiles = aml::Vector2{
                        static_cast<float>(size_in_pixels.x) / global_tile_size::get(),
                        static_cast<float>(size_in_pixels.y) / global_tile_size::get()};

                    aml::Matrix4 model = aml::Matrix4::identity;

                    // Apply sprite pivot
                    model *= aml::translate(aml::Vector3(-sprite->pivot, 1));
                    // Translate by position of entity
                    model *= aml::translate(aml::Vector3(entity->pos));
                    // Scale accordingly
                    model *= aml::scale(aml::Vector3{size_in_tiles, 1});

                    glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
                    glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
                    glUniformMatrix4fv(4, 1, GL_FALSE,
                                       lightSpaceMatrix.get_raw());        // Light space matrix
                    glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw()); // View matrix
                    glUniform2f(6, sprite->uv_min.x, sprite->uv_min.y);
                    glUniform2f(7, sprite->uv_max.x, sprite->uv_max.y);

                    constexpr int quad_verts = 2 * 3;
                    glDrawArrays(GL_TRIANGLES, 0, quad_verts);
                }
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

namespace detail {
static void draw_map(assets::Map const& map) {
    aml::Matrix4 model = aml::Matrix4::identity;
    model *= aml::scale({static_cast<float>(map.width), static_cast<float>(map.height),
                         static_cast<float>(global_tile_size::get())});

    glActiveTexture(GL_TEXTURE0);
    // Draw each layer
    for (auto& _l : map.layers) {
        auto layer = _l.get();
        if (!layer->visible)
            continue;

        glBindVertexArray(layer->get_mesh().get()->vao);
        glBindTexture(GL_TEXTURE_2D, layer->tileset.get()->texture.get()->handle);

        glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());

        glDrawArrays(GL_TRIANGLES, 0, layer->get_mesh().get()->vertex_count);
    }
}
} // namespace detail

} // namespace arpiyi::renderer