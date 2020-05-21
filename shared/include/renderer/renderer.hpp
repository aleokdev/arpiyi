#ifndef ARPIYI_RENDERER_HPP
#define ARPIYI_RENDERER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"
#include <anton/math/vector2.hpp>

namespace aml = anton::math;

struct GLFWwindow;
typedef void* ImTextureID;

namespace arpiyi::assets {
struct Map;
struct Sprite;
struct Mesh;
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
    /// you want to actually destroy the texture.#m
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
    [[nodiscard]] u32 width() const;
    [[nodiscard]] u32 height() const;
    [[nodiscard]] ColorType color_type() const;
    [[nodiscard]] FilteringMethod filter() const;
    [[nodiscard]] ImTextureID imgui_id() const;

    /// Loads a RGBA texture from a file path, and returns a handle to it. If there were any
    /// problems loading it, the TextureHandle returned won't be initialized to a value and its
    /// exists() will return false.
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

    assets::Mesh finish() const;

private:
    struct impl;
    std::unique_ptr<impl> p_impl;
};

class RenderMapContext {
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

class RenderTilesetContext {
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

    Framebuffer const& get_window_framebuffer();

    void start_frame();
    void finish_frame();

private:
    /// This is just a messy workaround needed to access private members of Texture. This will be
    /// removed in the future when I refactor the entire renderer, don't worry about it.
    static void draw_meshes(std::unordered_map<u64, assets::Mesh> const& batches);
    GLFWwindow* const window;
    struct impl;
    std::unique_ptr<impl> p_impl;
};

} // namespace arpiyi::renderer

#endif // ARPIYI_RENDERER_HPP
