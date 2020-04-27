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

enum class InputKey {
    k_0 = GLFW_KEY_0,
    k_1 = GLFW_KEY_1,
    k_2 = GLFW_KEY_2,
    k_3 = GLFW_KEY_3,
    k_4 = GLFW_KEY_4,
    k_5 = GLFW_KEY_5,
    k_6 = GLFW_KEY_6,
    k_7 = GLFW_KEY_7,
    k_8 = GLFW_KEY_8,
    k_9 = GLFW_KEY_9,
    k_A = GLFW_KEY_A,
    k_B = GLFW_KEY_B,
    k_C = GLFW_KEY_C,
    k_D = GLFW_KEY_D,
    k_E = GLFW_KEY_E,
    k_F = GLFW_KEY_F,
    k_G = GLFW_KEY_G,
    k_H = GLFW_KEY_H,
    k_I = GLFW_KEY_I,
    k_J = GLFW_KEY_J,
    k_K = GLFW_KEY_K,
    k_L = GLFW_KEY_L,
    k_M = GLFW_KEY_M,
    k_N = GLFW_KEY_N,
    k_O = GLFW_KEY_O,
    k_P = GLFW_KEY_P,
    k_Q = GLFW_KEY_Q,
    k_R = GLFW_KEY_R,
    k_S = GLFW_KEY_S,
    k_T = GLFW_KEY_T,
    k_U = GLFW_KEY_U,
    k_V = GLFW_KEY_V,
    k_W = GLFW_KEY_W,
    k_X = GLFW_KEY_X,
    k_Y = GLFW_KEY_Y,
    k_Z = GLFW_KEY_Z,
    k_SLASH = GLFW_KEY_SLASH,
    k_COMMA = GLFW_KEY_COMMA,
    k_DOT = GLFW_KEY_PERIOD,
    k_SEMICOLON = GLFW_KEY_SEMICOLON,
    k_LEFT_BRACKET = GLFW_KEY_LEFT_BRACKET,
    k_RIGHT_BRACKET = GLFW_KEY_RIGHT_BRACKET,
    k_MINUS = GLFW_KEY_MINUS,
    k_EQUALS = GLFW_KEY_EQUAL,
    k_APOSTROPHE = GLFW_KEY_APOSTROPHE,
    k_TICK = GLFW_KEY_GRAVE_ACCENT,
    k_BACKSLASH = GLFW_KEY_BACKSLASH,
    k_F1 = GLFW_KEY_F1,
    k_F2 = GLFW_KEY_F2,
    k_F3 = GLFW_KEY_F3,
    k_F4 = GLFW_KEY_F4,
    k_F5 = GLFW_KEY_F5,
    k_F6 = GLFW_KEY_F6,
    k_F7 = GLFW_KEY_F7,
    k_F8 = GLFW_KEY_F8,
    k_F9 = GLFW_KEY_F9,
    k_F10 = GLFW_KEY_F10,
    k_F11 = GLFW_KEY_F11,
    k_F12 = GLFW_KEY_F12,
    k_LEFT = GLFW_KEY_LEFT,
    k_RIGHT = GLFW_KEY_RIGHT,
    k_UP = GLFW_KEY_UP,
    k_DOWN = GLFW_KEY_DOWN,
    k_CAPS_LOCK = GLFW_KEY_CAPS_LOCK,
    k_TAB = GLFW_KEY_TAB,
    k_PAGE_UP = GLFW_KEY_PAGE_UP,
    k_PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
    k_HOME = GLFW_KEY_HOME,
    k_END = GLFW_KEY_END,
    k_INSERT = GLFW_KEY_INSERT,
    k_DELETE = GLFW_KEY_DELETE,
    k_BACKSPACE = GLFW_KEY_BACKSPACE,
    k_SPACE = GLFW_KEY_SPACE,
    k_ENTER = GLFW_KEY_ENTER,
    k_ESCAPE = GLFW_KEY_ESCAPE,
    k_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
    k_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
    k_LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
    k_RIGHT_CONTROL = GLFW_KEY_RIGHT_CONTROL,
    k_LEFT_ALT = GLFW_KEY_LEFT_ALT,
    k_RIGHT_ALT = GLFW_KEY_RIGHT_ALT,
    k_NUMPAD_0 = GLFW_KEY_KP_0,
    k_NUMPAD_1 = GLFW_KEY_KP_1,
    k_NUMPAD_2 = GLFW_KEY_KP_2,
    k_NUMPAD_3 = GLFW_KEY_KP_3,
    k_NUMPAD_4 = GLFW_KEY_KP_4,
    k_NUMPAD_5 = GLFW_KEY_KP_5,
    k_NUMPAD_6 = GLFW_KEY_KP_6,
    k_NUMPAD_7 = GLFW_KEY_KP_7,
    k_NUMPAD_8 = GLFW_KEY_KP_8,
    k_NUMPAD_9 = GLFW_KEY_KP_9,
    k_NUMPAD_ADD = GLFW_KEY_KP_ADD,
    k_NUMPAD_SUBTRACT = GLFW_KEY_KP_SUBTRACT,
    k_NUMPAD_DIVIDE = GLFW_KEY_KP_DIVIDE,
    k_NUMPAD_MULTIPLY = GLFW_KEY_KP_MULTIPLY,
    k_NUMPAD_DECIMAL = GLFW_KEY_KP_DECIMAL,
    k_NUMPAD_ENTER = GLFW_KEY_KP_ENTER,
    k_NUMLOCK = GLFW_KEY_NUM_LOCK
};

void define_input_table(sol::state_view& s) {
    /* clang-format off */
    sol::table game_table = s["game"];
    sol::table input_table = game_table["input"] = sol::new_table();
    input_table.new_enum<InputKey>("keys", {
        {"0", InputKey::k_0},
        {"1", InputKey::k_1},
        {"2", InputKey::k_2},
        {"3", InputKey::k_3},
        {"4", InputKey::k_4},
        {"5", InputKey::k_5},
        {"6", InputKey::k_6},
        {"7", InputKey::k_7},
        {"8", InputKey::k_8},
        {"9", InputKey::k_9},
        {"A", InputKey::k_A},
        {"B", InputKey::k_B},
        {"C", InputKey::k_C},
        {"D", InputKey::k_D},
        {"E", InputKey::k_E},
        {"F", InputKey::k_F},
        {"G", InputKey::k_G},
        {"H", InputKey::k_H},
        {"I", InputKey::k_I},
        {"J", InputKey::k_J},
        {"K", InputKey::k_K},
        {"L", InputKey::k_L},
        {"M", InputKey::k_M},
        {"N", InputKey::k_N},
        {"O", InputKey::k_O},
        {"P", InputKey::k_P},
        {"Q", InputKey::k_Q},
        {"R", InputKey::k_R},
        {"S", InputKey::k_S},
        {"T", InputKey::k_T},
        {"U", InputKey::k_U},
        {"V", InputKey::k_V},
        {"W", InputKey::k_W},
        {"X", InputKey::k_X},
        {"Y", InputKey::k_Y},
        {"Z", InputKey::k_Z},
        {"SLASH", InputKey::k_SLASH},
        {"COMMA", InputKey::k_COMMA},
        {"DOT", InputKey::k_DOT},
        {"SEMICOLON", InputKey::k_SEMICOLON},
        {"LEFT_BRACKET", InputKey::k_LEFT_BRACKET},
        {"RIGHT_BRACKET", InputKey::k_RIGHT_BRACKET},
        {"MINUS", InputKey::k_MINUS},
        {"EQUALS", InputKey::k_EQUALS},
        {"APOSTROPHE", InputKey::k_APOSTROPHE},
        {"TICK", InputKey::k_TICK},
        {"BACKWARD_SLASH", InputKey::k_BACKSLASH},
        {"F1", InputKey::k_F1},
        {"F2", InputKey::k_F2},
        {"F3", InputKey::k_F3},
        {"F4", InputKey::k_F4},
        {"F5", InputKey::k_F5},
        {"F6", InputKey::k_F6},
        {"F7", InputKey::k_F7},
        {"F8", InputKey::k_F8},
        {"F9", InputKey::k_F9},
        {"F10", InputKey::k_F10},
        {"F11", InputKey::k_F11},
        {"F12", InputKey::k_F12},
        {"LEFT", InputKey::k_LEFT},
        {"RIGHT", InputKey::k_RIGHT},
        {"UP", InputKey::k_UP},
        {"DOWN", InputKey::k_DOWN},
        {"CAPS_LOCK", InputKey::k_CAPS_LOCK},
        {"TAB", InputKey::k_TAB},
        {"PAGE_UP", InputKey::k_PAGE_UP},
        {"PAGE_DOWN", InputKey::k_PAGE_DOWN},
        {"HOME", InputKey::k_HOME},
        {"END", InputKey::k_END},
        {"INSERT", InputKey::k_INSERT},
        {"DELETE", InputKey::k_DELETE},
        {"BACKSPACE", InputKey::k_BACKSPACE},
        {"SPACE", InputKey::k_SPACE},
        {"ENTER", InputKey::k_ENTER},
        {"ESCAPE", InputKey::k_ESCAPE},
        {"LEFT_SHIFT", InputKey::k_LEFT_SHIFT},
        {"RIGHT_SHIFT", InputKey::k_RIGHT_SHIFT},
        {"LEFT_CONTROL", InputKey::k_LEFT_CONTROL},
        {"RIGHT_CONTROL", InputKey::k_RIGHT_CONTROL},
        {"LEFT_ALT", InputKey::k_LEFT_ALT},
        {"RIGHT_ALT", InputKey::k_RIGHT_ALT},
        {"NUMPAD_0", InputKey::k_NUMPAD_0},
        {"NUMPAD_1", InputKey::k_NUMPAD_1},
        {"NUMPAD_2", InputKey::k_NUMPAD_2},
        {"NUMPAD_3", InputKey::k_NUMPAD_3},
        {"NUMPAD_4", InputKey::k_NUMPAD_4},
        {"NUMPAD_5", InputKey::k_NUMPAD_5},
        {"NUMPAD_6", InputKey::k_NUMPAD_6},
        {"NUMPAD_7", InputKey::k_NUMPAD_7},
        {"NUMPAD_8", InputKey::k_NUMPAD_8},
        {"NUMPAD_9", InputKey::k_NUMPAD_9},
        {"NUMPAD_ADD", InputKey::k_NUMPAD_ADD},
        {"NUMPAD_SUBTRACT", InputKey::k_NUMPAD_SUBTRACT},
        {"NUMPAD_DIVIDE", InputKey::k_NUMPAD_DIVIDE},
        {"NUMPAD_MULTIPLY", InputKey::k_NUMPAD_MULTIPLY},
        {"NUMPAD_DECIMAL", InputKey::k_NUMPAD_DECIMAL},
        {"NUMPAD_ENTER", InputKey::k_NUMPAD_ENTER},
        {"NUMLOCK", InputKey::k_NUMLOCK}
    });
    input_table.new_usertype<KeyState>("KeyState",
        "held", sol::readonly(&KeyState::held),
             "just_pressed", sol::readonly(&KeyState::just_pressed),
             "just_released", sol::readonly(&KeyState::just_released)
        );

    input_table.set_function("get_key_state", &get_key_state);

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
    define_input_table(s);
}

} // namespace arpiyi::api