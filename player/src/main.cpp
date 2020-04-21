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

namespace fs = std::filesystem;
using namespace arpiyi;

sol::state lua;

void drawLuaStateInspectorTable(lua_State* state) {
    ImGui::Indent();
    lua_pushnil(state);
    while (lua_next(state, -2) != 0) { // key(-1) is replaced by the next key(-1) in table(-2)
        /* uses 'key' (at index -2) and 'value' (at index -1) */

        const char* keyName;
        const char* valueData;

        // Duplicate the values before converting them to string because tolstring can affect the
        // actual data in the stack
        {
            lua_pushvalue(state, -2);
            keyName = lua_tostring(state, -1);
            lua_pop(state, 1);
        }

        {
            lua_pushvalue(state, -1);
            valueData = lua_tostring(state, -1);

            std::string final_string;
            if (valueData == nullptr) {
                // lua_tolstring returns NULL if the value is not a number or a string, so let's get
                // the address instead.
                const void* address = lua_topointer(state, -1);
                std::ostringstream addressStr;
                addressStr << address;

                final_string.append("<");
                if (lua_iscfunction(state, -1))
                    final_string.append("C Function");
                else if (lua_isthread(state, -1))
                    final_string.append("Thread");
                else if (lua_isuserdata(state, -1))
                    final_string.append("Userdata");
                final_string.append(" @ 0x");
                final_string.append(addressStr.str());
                final_string.append(">");
                valueData = final_string.c_str();
            }

            lua_pop(state, 1); // Remove duplicate

            if (lua_istable(state, -1)) {
                if (ImGui::CollapsingHeader(keyName))
                    drawLuaStateInspectorTable(state);
                else {
                    /* removes 'value'; keeps 'key' for next iteration */
                    lua_pop(state, 1); // remove value(-1), now key on top at(-1)
                }
            } else {
                ImGui::Text("%s - %s", keyName, valueData);

                /* removes 'value'; keeps 'key' for next iteration */
                lua_pop(state, 1); // remove value(-1), now key on top at(-1)
            }
        }
    }
    lua_pop(state, 1); // remove starting table val
    ImGui::Unindent();
}

void DrawLuaStateInspector(lua_State* state, bool* p_open) {
    if (!ImGui::Begin("State Inspector", p_open, 0)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("GLOBALS")) {
        lua_pushglobaltable(state);
        drawLuaStateInspectorTable(state);
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

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "You must supply a valid arpiyi project path to load." << std::endl;
        return -1;
    }

    if (!glfwInit()) {
        std::cerr << "Couldn't init GLFW." << std::endl;
        return -1;
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
        return -1;
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

    fs::path project_path = fs::absolute(argv[1]);
    serializer::load_assets<assets::Texture>(project_path, callback);
    serializer::load_assets<assets::Sprite>(project_path, callback);
    serializer::load_assets<assets::Entity>(project_path, callback);
    serializer::load_assets<assets::Script>(project_path, callback);
    serializer::load_assets<assets::Map>(project_path, callback);
    std::cout << "Finished loading." << std::endl;

    arpiyi::api::define_api(lua);
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