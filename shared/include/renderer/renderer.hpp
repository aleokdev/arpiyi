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
struct Texture;
} // namespace arpiyi::assets

namespace arpiyi::renderer {

class Renderer;
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
    [[nodiscard]] ImTextureID get_imgui_id() const;

private:
    void destroy();

    friend class Renderer;
    friend class RenderMapContext;
    friend class RenderTilesetContext;
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
    GLFWwindow* const window;
    struct impl;
    std::unique_ptr<impl> p_impl;
};

} // namespace arpiyi::renderer

#endif // ARPIYI_RENDERER_HPP
