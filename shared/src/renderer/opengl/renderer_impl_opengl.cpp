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
#include <stb_image_write.h>
#include <stb_image.h>

namespace arpiyi::renderer {

struct TextureHandle::impl {
    u32 width;
    u32 height;
    ColorType color_type;
    FilteringMethod filter;
    u32 handle = 0;
};

TextureHandle::TextureHandle() : p_impl(std::make_unique<impl>()) {}
TextureHandle::~TextureHandle() {}
TextureHandle::TextureHandle(TextureHandle const& other) : p_impl(std::make_unique<impl>()) {
    *p_impl = *other.p_impl;
}
TextureHandle& TextureHandle::operator=(TextureHandle const& other) {
    if (this != &other) {
        *p_impl = *other.p_impl;
    }
    return *this;
}

ImTextureID TextureHandle::imgui_id() const {
    // Make sure the texture exists.
    assert(exists());
    return reinterpret_cast<void*>(p_impl->handle);
}
bool TextureHandle::exists() const { return p_impl->handle != 0; }
u32 TextureHandle::width() const { return p_impl->width; }
u32 TextureHandle::height() const { return p_impl->height; }
TextureHandle::ColorType TextureHandle::color_type() const { return p_impl->color_type; }
TextureHandle::FilteringMethod TextureHandle::filter() const { return p_impl->filter; }

void TextureHandle::init(
    u32 width, u32 height, ColorType type, FilteringMethod filter, const void* data) {
    // Remember to call unload() first if you want to reload the texture with different parameters.
    assert(!exists());
    glGenTextures(1, (u32*)&p_impl->handle);
    glBindTexture(GL_TEXTURE_2D, p_impl->handle);

    p_impl->width = width;
    p_impl->height = height;
    p_impl->color_type = type;
    p_impl->filter = filter;
    switch (type) {
        case (ColorType::rgba):
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         data);
            break;
        case (ColorType::depth):
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT,
                         GL_FLOAT, data);
            break;
        default: assert(false && "Unknown ColorType");
    }
    switch (filter) {
        case FilteringMethod::point:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
        case FilteringMethod::linear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
            break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border_color[4] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
}
TextureHandle TextureHandle::from_file(fs::path const& path, FilteringMethod filter, bool flip) {
    stbi_set_flip_vertically_on_load(flip);
    int w, h, channels;
    TextureHandle tex;
    unsigned char* data = stbi_load(path.generic_string().c_str(), &w, &h, &channels, 4);

    tex.init(w, h, ColorType::rgba, filter, data);

    stbi_image_free(data);
    return tex;
}

void TextureHandle::unload() {
    // glDeleteTextures ignores 0s (not created textures)
    glDeleteTextures(1, &p_impl->handle);
    p_impl->handle = 0;
}

struct MeshHandle::impl {
    u32 vbo = 0;
    u32 vao = 0;
    u32 vertex_count;
};

MeshHandle::MeshHandle() : p_impl(std::make_unique<impl>()) {}
MeshHandle::~MeshHandle() {}
MeshHandle::MeshHandle(MeshHandle const& other) : p_impl(std::make_unique<impl>()) {
    *p_impl = *other.p_impl;
}
MeshHandle& MeshHandle::operator=(MeshHandle const& other) {
    if (this != &other) {
        *p_impl = *other.p_impl;
    }
    return *this;
}

bool MeshHandle::exists() const {
    /// Only vao or vbo exist, but not both at once? Internal error!!
    assert((bool)(p_impl->vao) ^ (bool)(p_impl->vbo));
    return p_impl->vao;
}
void MeshHandle::unload() {
    // Zeros (non-existent meshes) are silently ignored
    glDeleteVertexArrays(1, &p_impl->vao);
    p_impl->vao = 0;
    glDeleteBuffers(1, &p_impl->vbo);
    p_impl->vbo = 0;
}

struct MeshBuilder::impl {
    std::vector<float> result;

    static constexpr auto sizeof_vertex = 5;
    static constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    static constexpr auto sizeof_quad = 2 * sizeof_triangle;
};

MeshBuilder::MeshBuilder() : p_impl(std::make_unique<impl>()) {}
MeshBuilder::~MeshBuilder() = default;

void MeshBuilder::add_sprite(assets::Sprite const& spr,
                             aml::Vector3 offset,
                             float vertical_slope,
                             float horizontal_slope) {
    const auto pivot = spr.pivot;
    const auto add_piece = [&](assets::Sprite::Piece const& piece) {
        auto& result = p_impl->result;
        const auto base_n = result.size();
        result.resize(result.size() + impl::sizeof_quad);
        const math::Rect2D pos_rect{{piece.destination.start.x - pivot.x + offset.x,
                                     piece.destination.start.y - pivot.y + offset.y},
                                    {piece.destination.end.x - pivot.x + offset.x,
                                     piece.destination.end.y - pivot.y + offset.y}};
        const math::Rect2D uv_rect = piece.source;
        const math::Rect2D z_map{{
                                     pos_rect.start.x * horizontal_slope + offset.z,
                                     pos_rect.start.y * vertical_slope + offset.z,
                                 },
                                 {
                                     pos_rect.end.x * horizontal_slope + offset.z,
                                     pos_rect.end.y * vertical_slope + offset.z,
                                 }};

        // First triangle //
        // First triangle //
        /* X pos 1st vertex */ result[base_n + 0] = pos_rect.start.x;
        /* Y pos 1st vertex */ result[base_n + 1] = pos_rect.start.y;
        /* Z pos 1st vertex */ result[base_n + 2] = z_map.start.x + z_map.start.y;
        /* X UV 1st vertex  */ result[base_n + 3] = uv_rect.start.x;
        /* Y UV 1st vertex  */ result[base_n + 4] = uv_rect.end.y;
        /* X pos 2nd vertex */ result[base_n + 5] = pos_rect.end.x;
        /* Y pos 2nd vertex */ result[base_n + 6] = pos_rect.start.y;
        /* Z pos 2nd vertex */ result[base_n + 7] = z_map.end.x + z_map.start.y;
        /* X UV 2nd vertex  */ result[base_n + 8] = uv_rect.end.x;
        /* Y UV 2nd vertex  */ result[base_n + 9] = uv_rect.end.y;
        /* X pos 3rd vertex */ result[base_n + 10] = pos_rect.start.x;
        /* Y pos 3rd vertex */ result[base_n + 11] = pos_rect.end.y;
        /* Z pos 3rd vertex */ result[base_n + 12] = z_map.start.x + z_map.end.y;
        /* X UV 2nd vertex  */ result[base_n + 13] = uv_rect.start.x;
        /* Y UV 2nd vertex  */ result[base_n + 14] = uv_rect.start.y;

        // Second triangle //
        /* X pos 1st vertex */ result[base_n + 15] = pos_rect.end.x;
        /* Y pos 1st vertex */ result[base_n + 16] = pos_rect.start.y;
        /* Z pos 1st vertex */ result[base_n + 17] = z_map.end.x + z_map.start.y;
        /* X UV 1st vertex  */ result[base_n + 18] = uv_rect.end.x;
        /* Y UV 1st vertex  */ result[base_n + 19] = uv_rect.end.y;
        /* X pos 2nd vertex */ result[base_n + 20] = pos_rect.end.x;
        /* Y pos 2nd vertex */ result[base_n + 21] = pos_rect.end.y;
        /* Z pos 2nd vertex */ result[base_n + 22] = z_map.end.x + z_map.end.y;
        /* X UV 2nd vertex  */ result[base_n + 23] = uv_rect.end.x;
        /* Y UV 2nd vertex  */ result[base_n + 24] = uv_rect.start.y;
        /* X pos 3rd vertex */ result[base_n + 25] = pos_rect.start.x;
        /* Y pos 3rd vertex */ result[base_n + 26] = pos_rect.end.y;
        /* Z pos 3rd vertex */ result[base_n + 27] = z_map.start.x + z_map.end.y;
        /* X UV 3rd vertex  */ result[base_n + 28] = uv_rect.start.x;
        /* Y UV 3rd vertex  */ result[base_n + 29] = uv_rect.start.y;
    };

    for (const auto& piece : spr.pieces) { add_piece(piece); }
}

MeshHandle MeshBuilder::finish() const {
    MeshHandle mesh;
    glGenVertexArrays(1, &mesh.p_impl->vao);
    glGenBuffers(1, &mesh.p_impl->vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh.p_impl->vbo);
    glBufferData(GL_ARRAY_BUFFER, p_impl->result.size() * sizeof(float), p_impl->result.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(mesh.p_impl->vao);
    // Vertex Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, mesh.p_impl->vbo, 0, impl::sizeof_vertex * sizeof(float));
    glVertexAttribBinding(0, 0);
    // UV Positions
    glEnableVertexAttribArray(1); // location 1
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glBindVertexBuffer(1, mesh.p_impl->vbo, 0, impl::sizeof_vertex * sizeof(float));
    glVertexAttribBinding(1, 1);

    mesh.p_impl->vertex_count = p_impl->result.size() / impl::sizeof_vertex;
    return mesh;
}

struct Renderer::impl {
    MeshHandle quad_mesh;
    Handle<assets::Shader> shaded_tile_shader;
    Handle<assets::Shader> basic_tile_shader;
    Handle<assets::Shader> entity_shader;
    Handle<assets::Shader> depth_shader;
    Handle<assets::Shader> grid_shader;
    unsigned int tile_shader_tile_tex_location;
    unsigned int tile_shader_shadow_tex_location;
    unsigned int entity_shader_tile_tex_location;
    unsigned int entity_shader_shadow_tex_location;

    Framebuffer window_framebuffer;
};

struct Framebuffer::impl {
    unsigned int handle;
    TextureHandle tex;
};

Renderer::~Renderer() = default;

Renderer::Renderer(GLFWwindow* _w) : window(_w), p_impl(std::make_unique<impl>()) {
    {
        MeshBuilder builder;
        assets::Sprite spr;
        spr.pieces = {{{{0,0},{1,1}}, {{0,0},{1,1}}}};
        builder.add_sprite(spr, {0,0,0}, 0, 0);
        p_impl->quad_mesh = builder.finish();
    }

    p_impl->shaded_tile_shader =
        asset_manager::load<assets::Shader>({"data/shaded_tile.vert", "data/shaded_tile.frag"});
    p_impl->basic_tile_shader =
        asset_manager::load<assets::Shader>({"data/basic_tile.vert", "data/basic_tile.frag"});

    p_impl->depth_shader =
        asset_manager::load<assets::Shader>({"data/depth.vert", "data/depth.frag"});
    p_impl->entity_shader =
        asset_manager::load<assets::Shader>({"data/shaded_tile.vert", "data/shaded_tile_uv.frag"});
    p_impl->grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    p_impl->tile_shader_tile_tex_location =
        glGetUniformLocation(p_impl->shaded_tile_shader.get()->handle, "tile");
    p_impl->tile_shader_shadow_tex_location =
        glGetUniformLocation(p_impl->shaded_tile_shader.get()->handle, "shadow");
    p_impl->entity_shader_tile_tex_location =
        glGetUniformLocation(p_impl->entity_shader.get()->handle, "tile");
    p_impl->entity_shader_shadow_tex_location =
        glGetUniformLocation(p_impl->entity_shader.get()->handle, "shadow");

    p_impl->window_framebuffer.p_impl->handle = 0;
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

Framebuffer const& Renderer::get_window_framebuffer() {
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

Framebuffer::Framebuffer() : p_impl(std::make_unique<impl>()) {
    p_impl->handle = static_cast<unsigned int>(-1);
}

Framebuffer::Framebuffer(math::IVec2D size) : p_impl(std::make_unique<impl>()) { set_size(size); }

Framebuffer::Framebuffer(Framebuffer const& other) : p_impl(std::make_unique<impl>()) {
    p_impl->handle = other.p_impl->handle;
    p_impl->tex = other.p_impl->tex;
}
Framebuffer& Framebuffer::operator=(Framebuffer const& other) {
    if (&other == this)
        return *this;

    p_impl->handle = other.p_impl->handle;
    p_impl->tex = other.p_impl->tex;

    return *this;
}

void Framebuffer::set_size(math::IVec2D size) {
    auto& handle = p_impl->handle;
    auto& tex = p_impl->tex;
    tex.unload();
    tex.init(size.x, size.y, TextureHandle::ColorType::rgba, TextureHandle::FilteringMethod::point);
    if (handle != static_cast<u32>(-1))
        glDeleteFramebuffers(1, &handle);
    glCreateFramebuffers(1, &handle);

    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.p_impl->handle,
                           0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

math::IVec2D Framebuffer::get_size() const {
    return {static_cast<i32>(p_impl->tex.width()), static_cast<i32>(p_impl->tex.height())};
}

TextureHandle const& Framebuffer::texture() const { return p_impl->tex; }

Framebuffer::~Framebuffer() = default;

void Framebuffer::destroy() {
    if (glfwGetCurrentContext() == nullptr)
        return;
    p_impl->tex.unload();
    if (p_impl->handle != static_cast<unsigned int>(-1))
        glDeleteFramebuffers(1, &p_impl->handle);
}

struct RenderMapContext::impl {
    unsigned int raw_depth_fb;
    TextureHandle depth_tex;
    Handle<assets::Map> last_map;
    // u64 is a texture handle. This contains all the meshes that the map is composed of.
    // TODO: Define std::hash for Handle and use it here as a key
    std::unordered_map<u64, MeshHandle> meshes;
};

RenderMapContext::RenderMapContext(math::IVec2D shadow_resolution) :
    p_impl(std::make_unique<impl>()) {
    set_shadow_resolution(shadow_resolution);
}
RenderMapContext::~RenderMapContext() { output_fb.destroy(); }

void RenderMapContext::set_shadow_resolution(math::IVec2D size) {
    auto& shadow_fb_handle = p_impl->raw_depth_fb;
    auto& shadow_tex = p_impl->depth_tex;
    shadow_tex.unload();
    shadow_tex.init(size.x, size.y, TextureHandle::ColorType::depth,
                    TextureHandle::FilteringMethod::point);
    if (shadow_fb_handle != static_cast<unsigned int>(-1))
        glDeleteFramebuffers(1, &shadow_fb_handle);
    glCreateFramebuffers(1, &shadow_fb_handle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size.x, size.y, 0, GL_DEPTH_COMPONENT,
                 GL_FLOAT, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fb_handle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           shadow_tex.p_impl->handle, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}
math::IVec2D RenderMapContext::get_shadow_resolution() const {
    return {static_cast<i32>(p_impl->depth_tex.width()),
            static_cast<i32>(p_impl->depth_tex.height())};
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
    glUseProgram(p_impl->depth_shader.get()->handle);
    glUniformMatrix4fv(2, 1, GL_FALSE, lightSpaceMatrix.get_raw());
    draw_meshes(data.p_impl->meshes);

    // Regular image rendering
    glBindFramebuffer(GL_FRAMEBUFFER, data.output_fb.p_impl->handle);
    glViewport(0, 0, data.output_fb.get_size().x, data.output_fb.get_size().y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(p_impl->shaded_tile_shader.get()->handle);
    glUniform1i(p_impl->tile_shader_tile_tex_location, 0); // Set tile sampler2D to GL_TEXTURE0
    glUniform1i(p_impl->tile_shader_shadow_tex_location,
                1);                                           // Set shadow sampler2D to GL_TEXTURE1
    glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
    glUniformMatrix4fv(4, 1, GL_FALSE, lightSpaceMatrix.get_raw()); // Light space matrix
    glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw());          // View matrix
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.p_impl->handle);
    draw_meshes(data.p_impl->meshes);

    if (data.draw_grid) {
        // Draw mesh grid
        glUseProgram(p_impl->grid_shader.get()->handle);
        glBindVertexArray(p_impl->quad_mesh.p_impl->vao);
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
        glUseProgram(p_impl->shaded_tile_shader.get()->handle);
        glUniform1i(p_impl->entity_shader_tile_tex_location,
                    0); // Set tile sampler2D to GL_TEXTURE0
        glUniform1i(p_impl->entity_shader_shadow_tex_location,
                    1); // Set shadow sampler2D to GL_TEXTURE1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, data.p_impl->depth_tex.p_impl->handle);
        glBindVertexArray(p_impl->quad_mesh.p_impl->vao);
        glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw()); // Projection matrix
        glUniformMatrix4fv(4, 1, GL_FALSE,
                           lightSpaceMatrix.get_raw());        // Light space matrix
        glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw()); // View matrix
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

                    glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
                    glBindVertexArray(mesh.p_impl->vao);

                    glDrawArrays(GL_TRIANGLES, 0, mesh.p_impl->vertex_count);
                    mesh.unload();
                }
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

struct RenderTilesetContext::impl {
    Handle<assets::Tileset> last_tileset;
    MeshHandle tileset_mesh;
};

RenderTilesetContext::RenderTilesetContext() : p_impl(std::make_unique<impl>()) {}
RenderTilesetContext::~RenderTilesetContext() { output_fb.destroy(); }

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
    glUseProgram(p_impl->basic_tile_shader.get()->handle);
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
        glUseProgram(p_impl->grid_shader.get()->handle);
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
        glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());
        glUniformMatrix4fv(2, 1, GL_FALSE, t_proj_mat.get_raw());
        glUniformMatrix4fv(5, 1, GL_FALSE, cam_mat.get_raw());

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

        glUniformMatrix4fv(1, 1, GL_FALSE, model.get_raw());

        glDrawArrays(GL_TRIANGLES, 0, mesh.p_impl->vertex_count);
    }
}

} // namespace arpiyi::renderer