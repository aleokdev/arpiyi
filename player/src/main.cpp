/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "assets/entity.hpp"
#include "assets/map.hpp"
#include "assets/sprite.hpp"
#include "assets/texture.hpp"
#include "serializer.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>

#include "util/defs.hpp"

#include <imgui.h>
#include <sol/sol.hpp>
#include <lua.hpp>
#include "api/api.hpp"
#include "assets/script.hpp"

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

static void debug_callback(GLenum const source,
                           GLenum const type,
                           GLuint,
                           GLenum const severity,
                           GLsizei,
                           GLchar const* const message,
                           void const*) {
    auto stringify_source = [](GLenum const source) {
        switch (source) {
            case GL_DEBUG_SOURCE_API: return u8"API";
            case GL_DEBUG_SOURCE_APPLICATION: return u8"Application";
            case GL_DEBUG_SOURCE_SHADER_COMPILER: return u8"Shader Compiler";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return u8"Window System";
            case GL_DEBUG_SOURCE_THIRD_PARTY: return u8"Third Party";
            case GL_DEBUG_SOURCE_OTHER: return u8"Other";
        }
        ARPIYI_UNREACHABLE();
    };

    auto stringify_type = [](GLenum const type) {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR: return u8"Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return u8"Deprecated Behavior";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return u8"Undefined Behavior";
            case GL_DEBUG_TYPE_PORTABILITY: return u8"Portability";
            case GL_DEBUG_TYPE_PERFORMANCE: return u8"Performance";
            case GL_DEBUG_TYPE_MARKER: return u8"Marker";
            case GL_DEBUG_TYPE_PUSH_GROUP: return u8"Push Group";
            case GL_DEBUG_TYPE_POP_GROUP: return u8"Pop Group";
            case GL_DEBUG_TYPE_OTHER: return u8"Other";
        }
        ARPIYI_UNREACHABLE();
    };

    auto stringify_severity = [](GLenum const severity) {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH: return u8"Fatal Error";
            case GL_DEBUG_SEVERITY_MEDIUM: return u8"Error";
            case GL_DEBUG_SEVERITY_LOW: return u8"Warning";
            case GL_DEBUG_SEVERITY_NOTIFICATION: return u8"Note";
        }
        ARPIYI_UNREACHABLE();
    };

    std::cout << "[" << stringify_severity(severity) << ":" << stringify_type(type) << " in "
              << stringify_source(source) << "]: " << message << std::endl;
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
            file_data.startup_script = obj.value.GetUint64();
        }
    }

    return file_data;
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

    if (!glfwInit()) {
        std::cerr << "Couldn't init GLFW." << std::endl;
        return -2;
    }

    // Use OpenGL 4.5
    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    GLFWwindow* window = glfwCreateWindow(1080, 720, "Arpiyi Player", nullptr, nullptr);
    if (!window) {
        std::cerr
            << "Couldn't create window. Check your GPU drivers, as arpiyi requires OpenGL 4.5."
            << std::endl;
        return -3;
    }
    // Activate VSync and fix FPS
    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEBUG_OUTPUT);
    glCullFace(GL_FRONT_AND_BACK);

    glDebugMessageCallback(debug_callback, nullptr);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    const auto callback = [](auto str, auto progress) {
        std::cout << "Loading " << str << "... (" << std::setprecision(0) << std::fixed
                  << (progress * 100) << "%)" << std::endl;
    };

    ProjectFileData project_data = load_project_file(project_path);
    serializer::load_assets<assets::Texture>(project_path, callback);
    serializer::load_assets<assets::Sprite>(project_path, callback);
    serializer::load_assets<assets::Entity>(project_path, callback);
    serializer::load_assets<assets::Script>(project_path, callback);
    serializer::load_assets<assets::Map>(project_path, callback);
    std::cout << "Finished loading." << std::endl;

    arpiyi::api::define_api(lua);

    lua.open_libraries(sol::lib::base, sol::lib::debug);
    if(auto startup_script = project_data.startup_script.get()) {
        try {
            lua.script(startup_script->source);
        } catch(sol::error const& e) {
            std::cout << e.what() << std::endl;
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        for (const auto& [id, layer] : arpiyi::api::game_play_data::ScreenLayerContainer::get_instance().map) {
            layer.render_callback();
        }

        bool _ = true;
        DrawLuaStateInspector(lua.lua_state(), &_);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}