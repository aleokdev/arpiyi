#include "api/api.hpp"
#include "assets/entity.hpp"
#include "assets/sprite.hpp"
#include "util/math.hpp"

#include <anton/math/vector2.hpp>

namespace aml = anton::math;

namespace arpiyi::api {

ScreenLayer::ScreenLayer(GamePlayData& data, std::function<void(void)> const& render_callback) :
    render_callback(render_callback), game_data(&data) {
    to_front();
}

void ScreenLayer::to_front() {
    for (auto& layer : game_data->screen_layers) {
        if (layer->layer_order > layer_order)
            layer->layer_order--;
    }
    layer_order = static_cast<int>(game_data->screen_layers.size()) - 1;
}

void ScreenLayer::to_back() {
    for (auto& layer : game_data->screen_layers) {
        if (layer_order > layer->layer_order)
            layer->layer_order++;
    }
    layer_order = 0;
}

std::shared_ptr<ScreenLayer>& GamePlayData::new_screen_layer(sol::function const& render_callback) {
    return screen_layers.emplace_back(
        std::make_shared<ScreenLayer>(*this, [render_callback]() { render_callback(); }));
}

std::shared_ptr<ScreenLayer>& GamePlayData::add_default_map_layer() {
    return screen_layers.emplace_back(
        std::make_shared<ScreenLayer>(*this, map_screen_layer_render_cb));
}

std::vector<std::shared_ptr<ScreenLayer>> GamePlayData::get_all_screen_layers() {
    return screen_layers;
}

std::vector<std::shared_ptr<ScreenLayer>> GamePlayData::get_visible_screen_layers() {
    std::vector<std::shared_ptr<ScreenLayer>> layers;
    for (const auto& layer : screen_layers) {
        if (layer->visible)
            layers.emplace_back(layer);
    }

    return layers;
}

std::vector<std::shared_ptr<ScreenLayer>> GamePlayData::get_hidden_screen_layers() {
    std::vector<std::shared_ptr<ScreenLayer>> layers;
    for (const auto& layer : screen_layers) {
        if (!layer->visible)
            layers.emplace_back(layer);
    }

    return layers;
}

GamePlayData::GamePlayData() noexcept { cam = std::make_shared<Camera>(); }

void define_vec2(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<aml::Vector2>("Vec2",
            "x", &aml::Vector2::x,
            "y", &aml::Vector2::y
            );
    /* clang-format on */
}

void define_ivec2(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<math::IVec2D>("IVec2",
                                          "x", &math::IVec2D::x,
                                          "y", &math::IVec2D::y
    );
    /* clang-format on */
}

void define_camera(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<Camera>("CameraClass",
                                          "pos", &Camera::pos,
                                          "zoom", &Camera::zoom
    );
    /* clang-format on */
}

void define_sprite(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<assets::Sprite>("Sprite",
                                          "pivot", &assets::Sprite::pivot,
                                          "name", sol::readonly(&assets::Sprite::name),
                                          "pixel_size", sol::readonly_property(&assets::Sprite::get_size_in_pixels)
    );
    /* clang-format on */
}

void define_entity(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<assets::Entity>("Entity",
                                            "pos", &assets::Entity::pos,
                                            "name", &assets::Entity::name,
                                            "sprite", &assets::Entity::sprite
    );
    /* clang-format on */
}

void define_screen_layer(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    game_table.new_usertype<ScreenLayer>("ScreenLayer", "new", sol::no_constructor,
                                         "visible", &ScreenLayer::visible,
                                         "render_callback", &ScreenLayer::render_callback,
                                         "to_front", &ScreenLayer::to_front,
                                         "to_back", &ScreenLayer::to_back
    );
    /* clang-format on */
}

void define_game_play_data(GamePlayData& data, sol::state_view& s) {
    sol::table game_table = s["game"];

    game_table["camera"] = data.cam;
    game_table.set_function("new_screen_layer",
                            [&data](sol::function const& render_callback) -> decltype(auto) {
                                return data.new_screen_layer(render_callback);
                            });
    game_table.set_function("get_all_screen_layers",
                            [&data]() -> decltype(auto) { return data.get_all_screen_layers(); });
    game_table.set_function("get_visible_screen_layers", [&data]() -> decltype(auto) {
        return data.get_visible_screen_layers();
    });
    game_table.set_function("get_hidden_screen_layers", [&data]() -> decltype(auto) {
        return data.get_hidden_screen_layers();
    });
    game_table.set_function("add_default_map_layer",
                            [&data]() -> decltype(auto) { return data.add_default_map_layer(); });
}

void define_api(GamePlayData& data, sol::state_view& s) {
    s["game"] = sol::new_table();

    // Data data structures
    define_camera(s);
    define_vec2(s);
    define_ivec2(s);
    define_sprite(s);
    define_entity(s);
    define_screen_layer(s);
    define_game_play_data(data, s);
}

} // namespace arpiyi::api