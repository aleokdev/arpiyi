#ifndef ARPIYI_IMPL_TYPES_HPP
#define ARPIYI_IMPL_TYPES_HPP

#include "renderer/renderer.hpp"

#include <vector>

namespace arpiyi::renderer {

struct TextureHandle::impl {
    u32 width;
    u32 height;
    ColorType color_type;
    FilteringMethod filter;
    u32 handle = 0;
};

struct MeshHandle::impl {
    u32 vbo = 0;
    u32 vao = 0;
    u32 vertex_count;
};

struct ShaderHandle::impl {
    u32 handle = 0;
    u32 tile_tex_location = -1;
    u32 shadow_tex_location = -1;
};

struct MeshBuilder::impl {
    std::vector<float> result;

    static constexpr auto sizeof_vertex = 5;
    static constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    static constexpr auto sizeof_quad = 2 * sizeof_triangle;
};

struct Framebuffer::impl {
    unsigned int handle = -1;
    TextureHandle tex;
    [[nodiscard]] bool exists() const;
    void create_handle();
    void bind_texture();
};

struct Renderer::impl {
    MeshHandle quad_mesh;
    ShaderHandle lit_shader;
    ShaderHandle unlit_shader;
    ShaderHandle depth_shader;
    ShaderHandle grid_shader;
    Framebuffer shadow_depth_fb;
    unsigned int tile_shader_tile_tex_location;
    unsigned int tile_shader_shadow_tex_location;

    Framebuffer window_framebuffer;
};

}

#endif // ARPIYI_IMPL_TYPES_HPP
