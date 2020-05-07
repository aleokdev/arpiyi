#ifndef ARPIYI_RENDERER_HPP
#define ARPIYI_RENDERER_HPP

#include "asset_manager.hpp"
#include "util/math.hpp"
#include <anton/math/vector2.hpp>
#include <experimental/propagate_const>

namespace aml = anton::math;

struct GLFWwindow;
typedef void* ImTextureID;

namespace arpiyi::assets {
    struct Map;
    struct Texture;
}

namespace arpiyi::renderer {

class Renderer;
/// A generic framebuffer with a RGBA texture attached to it.
class Framebuffer {
public:
    Framebuffer();
    explicit Framebuffer(math::IVec2D size);
    ~Framebuffer();

    [[nodiscard]] math::IVec2D get_size() const;
    void set_size(math::IVec2D);
    [[nodiscard]] ImTextureID get_imgui_id() const;

private:
    friend class Renderer;
    struct impl;
    std::experimental::propagate_const<std::unique_ptr<impl>> p_impl;
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
    aml::Vector2 cam_pos;
    /// The zoom of the camera. Output tilesize will be multiplied by this value.
    float zoom;

    // Light settings
    /// The rotation of the light on the X axis (Vertical distortion), measured in radians.
    float x_light_rotation;
    /// The rotation of the light on the Z axis (Horizontal distortion), measured in radians.
    float z_light_rotation;

    // Misc settings
    /// Should a tile grid be drawn over the map?
    bool draw_grid;

private:
    friend class Renderer;
    struct impl;
    std::experimental::propagate_const<std::unique_ptr<impl>> p_impl;
};

class Renderer {
public:
    explicit Renderer(GLFWwindow*);
    ~Renderer();

    void draw_map(RenderMapContext const&);

    void start_frame();
    void finish_frame();

private:
    GLFWwindow* const window;
    struct impl;
    std::experimental::propagate_const<std::unique_ptr<impl>> p_impl;
};

}

#endif // ARPIYI_RENDERER_HPP
