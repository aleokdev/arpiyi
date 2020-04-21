#include "api/api.hpp"
#include "assets/entity.hpp"
#include "assets/sprite.hpp"
#include "util/math.hpp"

#include <anton/math/vector2.hpp>

namespace aml = anton::math;

namespace arpiyi::api {

ScreenLayer::ScreenLayer(std::function<void(void)> const& render_callback) :
    render_callback(render_callback) {
    to_front();
}
ScreenLayer::ScreenLayer(sol::function const& render_callback) :
    render_callback([render_callback]() { render_callback(); }) {
    to_front();
}

void ScreenLayer::to_front() {
    for (auto& [id, layer] : game_play_data::ScreenLayerContainer::get_instance().map) {
        if (layer.layer_order > layer_order)
            layer.layer_order--;
    }
    layer_order = game_play_data::ScreenLayerContainer::get_instance().map.size() - 1;
}

void ScreenLayer::to_back() {
    for (auto& [id, layer] : game_play_data::ScreenLayerContainer::get_instance().map) {
        if (layer_order > layer.layer_order)
            layer.layer_order++;
    }
    layer_order = 0;
}

std::vector<Handle<ScreenLayer>> ScreenLayer::get_all() {
    std::vector<Handle<ScreenLayer>> screen_layers;
    for (const auto& [id, layer] : game_play_data::ScreenLayerContainer::get_instance().map)
        screen_layers.emplace_back(id);
    return screen_layers;
}

std::vector<Handle<ScreenLayer>> ScreenLayer::get_visible() {
    std::vector<Handle<ScreenLayer>> screen_layers;
    for (const auto& [id, layer] : game_play_data::ScreenLayerContainer::get_instance().map) {
        if (layer.visible)
            screen_layers.emplace_back(id);
    }
    return screen_layers;
}

std::vector<Handle<ScreenLayer>> ScreenLayer::get_hidden() {
    std::vector<Handle<ScreenLayer>> screen_layers;
    for (const auto& [id, layer] : game_play_data::ScreenLayerContainer::get_instance().map) {
        if (!layer.visible)
            screen_layers.emplace_back(id);
    }
    return screen_layers;
}

void ScreenLayer::add_default_map_layer() {
    asset_manager::put<ScreenLayer>(ScreenLayer(map_screen_layer_render_cb));
}

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
    game_table["camera"] = game_play_data::cam;
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
    game_table.new_usertype<ScreenLayer>("ScreenLayer",
                                         "visible", &ScreenLayer::visible,
                                         "render_callback", &ScreenLayer::render_callback,
                                         "to_front", &ScreenLayer::to_front,
                                         "to_back", &ScreenLayer::to_back
    );
    /* clang-format on */
    game_table["ScreenLayer"]["get_all"] = &ScreenLayer::get_all;
    game_table["ScreenLayer"]["get_visible"] = &ScreenLayer::get_visible;
    game_table["ScreenLayer"]["get_hidden"] = &ScreenLayer::get_hidden;
    game_table["ScreenLayer"]["add_default_map_layer"] = &ScreenLayer::add_default_map_layer;
}

void define_api(sol::state_view& s) {
    s["game"] = sol::new_table();

    // Data data structures
    define_camera(s);
    define_vec2(s);
    define_ivec2(s);
    define_sprite(s);
    define_entity(s);
    define_screen_layer(s);
}

} // namespace arpiyi::api