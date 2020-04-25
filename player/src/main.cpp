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
#include "default_render_impls.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace arpiyi;

sol::state lua;

void draw_lua_state_inspector_table(sol::table const& table) {
    ImGui::Indent();
    for(const auto& [k, v] : table) {
        std::string k_val = k.as<std::string>();
        if(v.get_type() == sol::type::table) {
            if(ImGui::CollapsingHeader((k_val + "##" + std::to_string(table.registry_index())).c_str())) {
                sol::table tbl = v;
                draw_lua_state_inspector_table(tbl);
            }
        } else {
            std::string v_val;
            switch(v.get_type()) {
                case sol::type::lua_nil: v_val = "nil"; break;

                case sol::type::number:
                case sol::type::boolean:
                case sol::type::string: v_val = v.as<std::string>(); break;

                case sol::type::thread: v_val = "thread"; break;
                case sol::type::function: v_val = "function"; break;
                case sol::type::userdata: v_val = "userdata"; break;
                case sol::type::lightuserdata: v_val = "light_userdata"; break;
                case sol::type::table: break;
                default:
                    ARPIYI_UNREACHABLE();
            }

            ImGui::TextUnformatted(k_val.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(v_val.c_str());
        }
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
constexpr std::string_view metadata_path = "meta";

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
        std::cerr << "No arguments given. You must supply a valid arpiyi project path to load." << std::endl;
        return -1;
    }
    fs::path project_path = fs::absolute(argv[1]);
    std::cout << project_path.generic_string() << std::endl;
    if(!fs::is_directory(project_path)) {
        std::cerr << "Path given is not a folder. You must supply a valid arpiyi project path to load." << std::endl;
        return -1;
    }

    if(!window_manager::init())
        return -1;

    const auto callback = [](auto str, auto progress) {
        std::cout << "Loading " << str << "... (" << std::setprecision(0) << std::fixed
                  << (progress * 100) << "%)" << std::endl;
    };

    ProjectFileData project_data = load_project_file(project_path);
    for(std::size_t i = 0; i < serializer::serializable_assets; ++i)
        serializer::load_one_asset_type(i, project_path, callback);
    std::cout << "Finished loading." << std::endl;

    default_render_impls::init();
    arpiyi::api::define_api(game_data_manager::get_game_data(), lua);

    lua.open_libraries(sol::lib::base, sol::lib::debug, sol::lib::coroutine, sol::lib::math);
    sol::coroutine main_coroutine;
    if(auto startup_script = project_data.startup_script.get()) {
        try {
            sol::function f = lua.load(startup_script->source);
            main_coroutine = f;
            main_coroutine();
            if(main_coroutine.status() == sol::call_status::ok) {
                std::cout << "Main coroutine finished. Exiting..." << std::endl;
                return 0;
            }
        } catch(sol::error const& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << "Startup script was not able to execute on initialize. Exiting." << std::endl;
            return -1;
        }
    } else {
        std::cerr << "No startup script set. Exiting." << std::endl;
        return -1;
    }

    // debug: set current map to 0
    game_data_manager::get_game_data().current_map = Handle<assets::Map>((u64)0);

    glfwSetKeyCallback(window_manager::get_window(), key_callback);
    while (!glfwWindowShouldClose(window_manager::get_window())) {
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window_manager::get_window(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        main_coroutine();
        if(main_coroutine.status() == sol::call_status::ok) {
            std::cout << "Main coroutine finished. Exiting..." << std::endl;
            return 0;
        }

        for (const auto& layer : game_data_manager::get_game_data().screen_layers) {
            layer->render_callback();
        }

        if(show_state_inspector)
            DrawLuaStateInspector(lua.lua_state(), &show_state_inspector);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_manager::get_window());
    }

    glfwTerminate();

    return 0;
}