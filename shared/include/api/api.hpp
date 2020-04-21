#ifndef ARPIYI_API_HPP
#define ARPIYI_API_HPP

#include <anton/math/vector2.hpp>
#include <functional>
#include <sol/sol.hpp>
#include "asset_manager.hpp"

namespace aml = anton::math;

namespace arpiyi::api {

struct GamePlayData;
class Camera {
public:
    Camera() noexcept {}

    aml::Vector2 pos = {0,0};
    float zoom = 1;
};

class ScreenLayer;
class ScreenLayer {
public:
    explicit ScreenLayer(std::function<void(void)> const& render_callback);
    explicit ScreenLayer(sol::function const& render_callback);

    bool visible = true;
    std::function<void(void)> render_callback;
    std::size_t layer_order;

    void to_front();
    void to_back();

    static std::vector<Handle<ScreenLayer>> get_all();
    static std::vector<Handle<ScreenLayer>> get_visible();
    static std::vector<Handle<ScreenLayer>> get_hidden();

    static void add_default_map_layer();
};

extern void map_screen_layer_render_cb();

namespace game_play_data {
    static Camera cam;
    using ScreenLayerContainer = detail::AssetContainer<ScreenLayer>;
};

void define_api(sol::state_view& s);

}

namespace sol {
template<typename T> struct unique_usertype_traits<arpiyi::Handle<T>> {
    typedef T type;
    typedef arpiyi::Handle<T> actual_type;
    static const bool value = true;

    static bool is_null(const actual_type& ptr) { return ptr.get(); }
    static type* get(const actual_type& ptr) {
        auto val = ptr.get();
        return val ? const_cast<type*>(&*val) : nullptr;
    }
};
}

#endif // ARPIYI_API_HPP
