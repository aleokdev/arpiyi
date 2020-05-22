#ifndef ARPIYI_RENDERER_HPP
#define ARPIYI_RENDERER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"
#include <anton/math/vector2.hpp>
#include <anton/math/vector3.hpp>
#include <vector>

namespace aml = anton::math;

struct GLFWwindow;
typedef void* ImTextureID;

namespace arpiyi::assets {
struct Map;
struct Sprite;
} // namespace arpiyi::assets

namespace arpiyi::renderer {

class Renderer;

class TextureHandle {
public:
    enum class ColorType { rgba, depth };
    enum class FilteringMethod { point, linear };
    /// Doesn't actually create a texture -- If exists() is called before initializing it, it will
    /// return false. Call init() to initialize and create the texture.
    TextureHandle();
    /// The destructor will NOT unload the texture underneath. Remember to call unload() first if
    /// you want to actually destroy the texture.
    ~TextureHandle();
    TextureHandle(TextureHandle const&);
    TextureHandle& operator=(TextureHandle const&);

    /// IMPORTANT: This function will assert if called when a previous texture existed in this slot.
    /// Remember to call unload() first if you want to reload the texture with different parameters.
    void
    init(u32 width, u32 height, ColorType type, FilteringMethod filter, const void* data = nullptr);
    /// Destroys the texture underneath, or does nothing if it doesn't exist already.
    void unload();
    /// @returns True if the texture has been initialized and not unloaded.
    [[nodiscard]] bool exists() const;
    /// The width of the texture. If the texture doesn't exist, the result is
    /// implementation-defined.
    [[nodiscard]] u32 width() const;
    /// The height of the texture. If the texture doesn't exist, the result is
    /// implementation-defined.
    [[nodiscard]] u32 height() const;
    /// The color type of the texture. If the texture doesn't exist, the result is
    /// implementation-defined.
    [[nodiscard]] ColorType color_type() const;
    /// The filtering method of the texture. If the texture doesn't exist, the result is
    /// implementation-defined.
    [[nodiscard]] FilteringMethod filter() const;
    /// Returns an ImGui ID that represents the texture. If the texture doesn't exist, this function
    /// will assert.
    [[nodiscard]] ImTextureID imgui_id() const;

    /// Loads a RGBA texture from a file path, and returns a handle to it. If there were any
    /// problems loading it, the TextureHandle returned won't be initialized to a value and its
    /// exists() will return false.
    /// Must support the following formats: JPEG, PNG, TGA, BMP, PSD, GIF, HDR, PIC and PNM
    /// (Basically all the formats stb_image supports, which you can see in std_image.h)
    static TextureHandle from_file(fs::path const&, FilteringMethod filter, bool flip);

private:
    friend class Renderer;
    friend class Framebuffer;
    friend class RenderMapContext;
    friend class RenderTilesetContext;

    struct impl;
    std::unique_ptr<impl> p_impl;
};
/// A generic framebuffer with a RGBA texture attached to it.
class Framebuffer {
public:
    Framebuffer();
    explicit Framebuffer(math::IVec2D size);
    /// The destructor should NOT destroy the underlying framebuffer/handle. It is just put here so
    /// that the implementation links correctly.
    ~Framebuffer();
    Framebuffer(Framebuffer const&);
    Framebuffer& operator=(Framebuffer const&);

    [[nodiscard]] math::IVec2D get_size() const;
    void set_size(math::IVec2D);
    [[nodiscard]] TextureHandle const& texture() const;

private:
    void destroy();

    friend class Renderer;
    friend class RenderMapContext;
    friend class RenderTilesetContext;

    struct impl;
    std::unique_ptr<impl> p_impl;
};

struct MeshHandle {
    /// Meshes are only meant to be created by MeshBuilder. Otherwise you won't be able to put data
    /// in them.
    MeshHandle();
    /// The MeshHandle destructor won't actually unload the underlying mesh. Use unload() for that.
    ~MeshHandle();
    MeshHandle(MeshHandle const&);
    MeshHandle& operator=(MeshHandle const&);

    /// Returns true if the texture exists and has not been unloaded.
    [[nodiscard]] bool exists() const;
    /// Destroys the underlying mesh. Does nothing if the mesh was already unloaded previously.
    void unload();

private:
    friend class MeshBuilder;
    friend class Renderer;

    struct impl;
    std::unique_ptr<impl> p_impl;
};

struct ShaderHandle {
    /// Creates a blank shader handle. Does not really have an use outside of the renderer
    /// implementation.
    ShaderHandle();
    ~ShaderHandle();
    ShaderHandle(ShaderHandle const&);
    ShaderHandle& operator=(ShaderHandle const&);

    /// Returns true if the shader exists and has not been unloaded.
    [[nodiscard]] bool exists() const;
    /// Destroys the underlying shader. Does nothing if the shader was already unloaded previously.
    void unload();

    /// Loads a GLSL shader from two paths (One for the fragment shader and another one for the
    /// vertex one)
    static ShaderHandle from_file(fs::path const& vert_path, fs::path const& frag_path);

private:
    friend class Renderer;

    struct impl;
    std::unique_ptr<impl> p_impl;
};

struct Transform {
    aml::Vector3 position;
};

struct Camera {
    aml::Vector3 position;
    float zoom;
};

struct DrawCmd {
    TextureHandle texture;
    MeshHandle mesh;
    ShaderHandle shader;
    Transform transform;
    Camera camera;
};

using DrawCmdList = std::vector<DrawCmd>;

class MeshBuilder {
public:
    MeshBuilder();
    ~MeshBuilder();
    /// Adds a sprite to the mesh.
    /// @param spr The sprite. Takes into account its pivot.
    /// @param offset Where to place the sprite, in tile units.
    /// @param vertical_slope How many tile units to distort the sprite in the Z axis per Y unit.
    /// @param horizontal_slope How many tile units to distort the sprite in the Z axis per X unit.
    void add_sprite(assets::Sprite const& spr,
                    aml::Vector3 offset,
                    float vertical_slope,
                    float horizontal_slope);

    MeshHandle finish() const;

private:
    struct impl;
    std::unique_ptr<impl> p_impl;
};

class [[deprecated("Use DrawCmds and Renderer::draw instead.")]] RenderMapContext {
public:
    explicit RenderMapContext(math::IVec2D shadow_resolution);
    ~RenderMapContext();

    void set_shadow_resolution(math::IVec2D);
    [[nodiscard]] math::IVec2D get_shadow_resolution() const;

    Handle<assets::Map> map;
    Framebuffer output_fb;

    // Camera settings
    /// The camera position, in tiles.
    aml::Vector2 cam_pos = {0, 0};
    /// The zoom of the camera. Output tilesize will be multiplied by this value.
    float zoom = 1;

    // Light settings
    /// The rotation of the light on the X axis (Vertical distortion), measured in radians.
    float x_light_rotation = -0.5f;
    /// The rotation of the light on the Z axis (Horizontal distortion), measured in radians.
    float z_light_rotation = -0.5f;

    // Misc settings
    /// Should a tile grid be drawn over the map?
    bool draw_grid = false;
    /// Should the entities be drawn over the map?
    bool draw_entities = true;
    /// Should the internal map mesh be regenerated next update? (This is a trigger. It will be set
    /// back to false on next render cycle).
    mutable bool force_mesh_regeneration = false;

private:
    friend class Renderer;
    struct impl;
    std::unique_ptr<impl> p_impl;
};

class [[deprecated("Use DrawCmds and Renderer::draw instead.")]] RenderTilesetContext {
public:
    RenderTilesetContext();
    ~RenderTilesetContext();

    Handle<assets::Tileset> tileset;
    Framebuffer output_fb;

    // Camera settings
    /// The camera position, in tiles.
    aml::Vector2 cam_pos = {0, 0};
    /// The zoom of the camera. Output tilesize will be multiplied by this value.
    float zoom = 1;

    // Misc settings
    /// Should a tile grid be drawn over the tileset?
    bool draw_grid = true;

private:
    friend class Renderer;
    struct impl;
    std::unique_ptr<impl> p_impl;
};

class Renderer {
public:
    explicit Renderer(GLFWwindow*);
    ~Renderer();

    void draw_map(RenderMapContext const&);
    void draw_tileset(RenderTilesetContext const&);
    void draw(DrawCmdList const& draw_commands);

    // Returns the default lit shader. The handle will be valid until the renderer is destroyed.
    ShaderHandle lit_shader() const;
    // Returns the default unlit shader. The handle will be valid until the renderer is destroyed.
    ShaderHandle unlit_shader() const;

    Framebuffer const& get_window_framebuffer();

    void start_frame();
    void finish_frame();

private:
    /// This is just a messy workaround needed to access private members of Texture. This will be
    /// removed in the future when I refactor the entire renderer, don't worry about it.
    static void draw_meshes(std::unordered_map<u64, MeshHandle> const& batches);
    GLFWwindow* const window;
    struct impl;
    std::unique_ptr<impl> p_impl;
};

} // namespace arpiyi::renderer

#endif // ARPIYI_RENDERER_HPP
