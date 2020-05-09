/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <sol/sol.hpp>

#include "serializer.hpp"
#include "util/defs.hpp"
#include "api/api.hpp"
#include "assets/script.hpp"
#include "game_data_manager.hpp"
#include "window_manager.hpp"
#include "default_api_impls.hpp"
#include "global_tile_size.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace arpiyi;

sol::state lua;

void draw_lua_state_inspector_table(sol::table const& table, std::size_t id_to_use = 0) {
    ImGui::Indent();
    for (const auto& [k, v] : table) {
        std::string k_val = k.as<std::string>();
        if (v.get_type() == sol::type::table) {
            std::string str_id = k_val + "###" + std::to_string(id_to_use);
            if (ImGui::CollapsingHeader(str_id.c_str())) {
                sol::table tbl = v;
                draw_lua_state_inspector_table(tbl, id_to_use + 1);
            }
        } else {
            std::string v_val;
            switch (v.get_type()) {
                case sol::type::lua_nil: v_val = "nil"; break;

                case sol::type::number:
                case sol::type::boolean:
                case sol::type::string: v_val = v.as<std::string>(); break;

                case sol::type::thread: v_val = "thread"; break;
                case sol::type::function: v_val = "function"; break;
                case sol::type::userdata: v_val = "userdata"; break;
                case sol::type::lightuserdata: v_val = "light_userdata"; break;
                case sol::type::table: break;
                default: ARPIYI_UNREACHABLE();
            }

            ImGui::TextUnformatted(k_val.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(v_val.c_str());
        }
        id_to_use += 100;
    }
    ImGui::Unindent();
}

void DrawLuaStateInspector(sol::state_view const& state, bool* p_open) {
    if (!ImGui::Begin("State Inspector", p_open, 0)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("GLOBALS")) {
        draw_lua_state_inspector_table(state.globals());
    }

    ImGui::End();
}

namespace detail::project_file_definitions {

/// Path for storing files containing asset IDs and their location.

constexpr std::string_view tile_size_json_key = "tile_size";
constexpr std::string_view editor_version_json_key = "editor_version";
constexpr std::string_view startup_script_id_key = "startup_script_id";

} // namespace detail::project_file_definitions

struct ProjectFileData {
    u32 tile_size = 48;
    std::string editor_version;
    Handle<assets::Script> startup_script;
};

static ProjectFileData load_project_file(fs::path base_dir) {
    std::ifstream f(base_dir / "project.json");
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    ProjectFileData file_data;

    using namespace ::detail::project_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == tile_size_json_key.data()) {
            file_data.tile_size = obj.value.GetUint();
        } else if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        } else if (obj.name == startup_script_id_key.data()) {
            file_data.startup_script = Handle<assets::Script>(obj.value.GetUint64());
        }
    }

    return file_data;
}

static bool show_state_inspector = false;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (mods & GLFW_MOD_CONTROL && key == GLFW_KEY_K && action & GLFW_PRESS) {
        show_state_inspector = !show_state_inspector;
    }
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "No arguments given. You must supply a valid arpiyi project path to load."
                  << std::endl;
        return -1;
    }
    fs::path project_path = fs::absolute(argv[1]);
    std::cout << project_path.generic_string() << std::endl;
    if (!fs::is_directory(project_path)) {
        std::cerr
            << "Path given is not a folder. You must supply a valid arpiyi project path to load."
            << std::endl;
        return -1;
    }

    if (!window_manager::init())
        return -1;

    const auto callback = [](auto str, auto progress) {
        std::cout << "Loading " << str << "... (" << std::setprecision(0) << std::fixed
                  << (progress * 100) << "%)" << std::endl;
    };

    ProjectFileData project_data = load_project_file(project_path);
    global_tile_size::set(project_data.tile_size);
    for (std::size_t i = 0; i < serializer::serializable_assets; ++i)
        serializer::load_one_asset_type(i, project_path, callback);
    std::cout << "Finished loading." << std::endl;

    default_api_impls::init();
    arpiyi::api::define_api(game_data_manager::get_game_data(), lua);

    lua.open_libraries(sol::lib::base, sol::lib::debug, sol::lib::coroutine, sol::lib::math);
    sol::coroutine main_coroutine;
    sol::thread main_coroutine_thread = sol::thread::create(lua.lua_state());
    if (auto startup_script = project_data.startup_script.get()) {
        try {
            sol::function f = main_coroutine_thread.state().load(startup_script->source);
            main_coroutine = f;
        } catch (sol::error const& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "Startup script was not able to load on initialize. Exiting." << std::endl;
            return -1;
        }
    } else {
        std::cerr << "No startup script set. Exiting." << std::endl;
        return -1;
    }

    // debug: set current map to 0
    game_data_manager::get_game_data().current_map = Handle<assets::Map>((u64)0);

    assert(game_data_manager::get_game_data().current_map.get());

    glfwSetKeyCallback(window_manager::get_window(), key_callback);
    while (!glfwWindowShouldClose(window_manager::get_window())) {
        glfwPollEvents();

        window_manager::get_renderer().start_frame();

        main_coroutine();
        if (main_coroutine.status() == sol::call_status::ok) {
            std::cout << "Main coroutine finished. Searching for auto scripts..." << std::endl;
            struct {
                Handle<assets::Entity> parent_entity;
                Handle<assets::Script> script;
            } auto_obj;
            for (const auto& e : game_data_manager::get_game_data().current_map.get()->entities) {
                assert(e.get());
                for (const auto& script : e.get()->scripts) {
                    assert(script.get());
                    if (script.get()->trigger_type == assets::Script::TriggerType::t_auto) {
                        auto_obj.script = script;
                        auto_obj.parent_entity = e;
                        break;
                    } else if (script.get()->trigger_type ==
                               assets::Script::TriggerType::t_lp_auto) {
                        if (auto as = auto_obj.script.get()) {
                            if (as->trigger_type != assets::Script::TriggerType::t_auto) {
                                auto_obj.script = script;
                                auto_obj.parent_entity = e;
                            }
                        } else {
                            auto_obj.script = script;
                            auto_obj.parent_entity = e;
                        }
                    }
                }
            }
            if (auto a_s = auto_obj.script.get()) {
                assert(auto_obj.parent_entity.get());
                sol::function f = main_coroutine_thread.state().load(a_s->source);
                sol::environment script_env(main_coroutine_thread.state(), sol::create, lua.globals());
                script_env["entity"] = auto_obj.parent_entity;
                main_coroutine = f;
                script_env.set_on(main_coroutine);
            } else {
                std::cout << "Found no auto script source to replace dead main coroutine, exiting."
                          << std::endl;
                return -2;
            }
        }

        for (const auto& layer : game_data_manager::get_game_data().screen_layers) {
            layer->render_callback();
        }

        if (show_state_inspector)
            DrawLuaStateInspector(lua.lua_state(), &show_state_inspector);

        window_manager::get_renderer().finish_frame();
    }

    glfwTerminate();

    return 0;
}