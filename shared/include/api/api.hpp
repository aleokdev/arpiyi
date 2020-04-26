#ifndef ARPIYI_API_HPP
#define ARPIYI_API_HPP

#include "asset_manager.hpp"
#include "assets/map.hpp"
#include <anton/math/vector2.hpp>
#include <functional>
#include <memory>
#include <sol/sol.hpp>

namespace aml = anton::math;

namespace arpiyi::api {

struct GamePlayData;
class Camera {
public:
    Camera() noexcept {}

    aml::Vector2 pos = {0, 0};
    float zoom = 1;
};

class ScreenLayer;
class ScreenLayer {
public:
    explicit ScreenLayer(GamePlayData& game_data, std::function<void(void)> const& render_callback);

    bool visible = true;
    std::function<void(void)> render_callback;
    std::size_t layer_order;

    void to_front();
    void to_back();

private:
    GamePlayData* const game_data;
};

enum class InputKey;
struct KeyState {
    /// True if the key is pressed, false otherwise.
    bool held;
    /// True if the key press had begun on the last frame.
    bool just_pressed;
    /// True if the key release had begun on the last frame.
    bool just_released;
};

struct GamePlayData {
    GamePlayData() noexcept;
    // Lua public API functions/variables

    /// game.new_screen_layer(callback)
    std::shared_ptr<ScreenLayer>& new_screen_layer(sol::function const& render_callback);
    /// game.add_default_map_layer()
    std::shared_ptr<ScreenLayer>& add_default_map_layer();

    std::vector<std::shared_ptr<ScreenLayer>> get_all_screen_layers();
    std::vector<std::shared_ptr<ScreenLayer>> get_visible_screen_layers();
    std::vector<std::shared_ptr<ScreenLayer>> get_hidden_screen_layers();

    std::shared_ptr<Camera> cam;
    Handle<assets::Map> current_map;

    // Members not meant to be used with the lua API
    std::vector<std::shared_ptr<ScreenLayer>> screen_layers;
};

void define_api(GamePlayData& data, sol::state_view& s);

// Implementation-defined functions

/// The render callback of the screen layer added when the GamePlayData::add_default_map_layer
/// function is called.
extern void map_screen_layer_render_cb();

/// The implementation for the input.get_key_state function.
extern KeyState get_key_state(InputKey key);

} // namespace arpiyi::api

namespace sol {
template<typename T> struct unique_usertype_traits<arpiyi::Handle<T>> {
    typedef T type;
    typedef arpiyi::Handle<T> actual_type;
    static const bool value = true;

    static bool is_null(const actual_type& ptr) { return !ptr.get(); }
    static type* get(const actual_type& ptr) {
        auto val = ptr.get();
        return val ? const_cast<type*>(&*val) : nullptr;
    }
};
} // namespace sol

#endif // ARPIYI_API_HPP
