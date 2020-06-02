/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "renderer/opengl/impl_types.hpp"

#include "assets/map.hpp"
#include <anton/math/transform.hpp>
#include <global_tile_size.hpp>
#include <stb_image.h>
#include <memory>

namespace arpiyi::renderer {

TextureHandle::TextureHandle() : p_impl(std::make_unique<impl>()) {}
TextureHandle::~TextureHandle() = default;
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

MeshHandle::MeshHandle() : p_impl(std::make_unique<impl>()) {}
MeshHandle::~MeshHandle() = default;
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

ShaderHandle::ShaderHandle() : p_impl(std::make_unique<impl>()) {}
ShaderHandle::~ShaderHandle() = default;
ShaderHandle::ShaderHandle(ShaderHandle const& other) : p_impl(std::make_unique<impl>()) {
    *p_impl = *other.p_impl;
}
ShaderHandle& ShaderHandle::operator=(ShaderHandle const& other) {
    if (this != &other) {
        *p_impl = *other.p_impl;
    }
    return *this;
}

bool ShaderHandle::exists() const { return p_impl->handle; }
void ShaderHandle::unload() {
    glDeleteProgram(p_impl->handle);
    p_impl->handle = 0;
}

static unsigned int create_shader_stage(GLenum stage, fs::path const& path) {
    using namespace std::literals::string_literals;

    std::ifstream f(path);
    if (!f.good()) {
        throw std::runtime_error("Failed to open file: "s + path.generic_string());
    }
    std::string source((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    char* src = source.data();

    unsigned int shader = glCreateShader(stage);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infolog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infolog);
        throw std::runtime_error("Failed to compile shader:\n"s + source + "\nReason: "s + infolog);
    }

    return shader;
}

ShaderHandle ShaderHandle::from_file(fs::path const& vert_path, fs::path const& frag_path) {
    using namespace std::literals::string_literals;

    unsigned int vtx = create_shader_stage(GL_VERTEX_SHADER, vert_path);
    unsigned int frag = create_shader_stage(GL_FRAGMENT_SHADER, frag_path);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vtx);
    glAttachShader(prog, frag);

    glLinkProgram(prog);
    int success;
    char infolog[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, nullptr, infolog);
        throw std::runtime_error("Failed to link shader.\nReason: "s + infolog);
    }

    glDeleteShader(vtx);
    glDeleteShader(frag);

    ShaderHandle shader;
    shader.p_impl->handle = prog;
    shader.p_impl->tile_tex_location = glGetUniformLocation(prog, "tile");
    shader.p_impl->shadow_tex_location = glGetUniformLocation(prog, "shadow");
    return shader;
}

MeshBuilder::MeshBuilder() : p_impl(std::make_unique<impl>()) {}
MeshBuilder::~MeshBuilder() = default;
MeshBuilder::MeshBuilder(MeshBuilder const& other) : p_impl(std::make_unique<impl>()) {
    *p_impl = *other.p_impl;
}
MeshBuilder& MeshBuilder::operator=(MeshBuilder const& other) {
    if (this != &other) {
        *p_impl = *other.p_impl;
    }
    return *this;
}

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

Framebuffer::Framebuffer() : p_impl(std::make_unique<impl>()) {
    p_impl->handle = static_cast<unsigned int>(-1);
}
Framebuffer::Framebuffer(TextureHandle const& texture) : p_impl(std::make_unique<impl>()) {
    p_impl->create_handle();
    p_impl->tex = texture;
    p_impl->bind_texture();
}

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

void Framebuffer::impl::create_handle() {
    if (exists())
        glDeleteFramebuffers(1, &handle);
    glCreateFramebuffers(1, &handle);
}

void Framebuffer::impl::bind_texture() {
    assert(exists());
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    switch (tex.color_type()) {
        case TextureHandle::ColorType::rgba:
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   tex.p_impl->handle, 0);
            depth_tex.unload();
            depth_tex.init(tex.width(), tex.height(), TextureHandle::ColorType::depth, TextureHandle::FilteringMethod::point);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.p_impl->handle, 0);
            break;
        case TextureHandle::ColorType::depth:
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   tex.p_impl->handle, 0);
            break;
        default: assert(false && "Unknown color type"); return;
    }

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool Framebuffer::impl::exists() const { return handle != static_cast<u32>(-1); }

bool Framebuffer::exists() const { return p_impl->exists(); }

void Framebuffer::resize(math::IVec2D size) {
    assert(exists());
    auto& tex = p_impl->tex;
    const auto prev_color_type = tex.color_type();
    const auto prev_filter_type = tex.filter();
    tex.unload();
    tex.init(size.x, size.y, prev_color_type, prev_filter_type);
    p_impl->bind_texture();
}

TextureHandle const& Framebuffer::texture() const { return p_impl->tex; }

Framebuffer::~Framebuffer() = default;

void Framebuffer::unload() {
    if (glfwGetCurrentContext() == nullptr)
        return;
    p_impl->tex.unload();
    if (p_impl->handle != static_cast<unsigned int>(-1))
        glDeleteFramebuffers(1, &p_impl->handle);
}

} // namespace arpiyi::renderer