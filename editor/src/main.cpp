/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "editor/editor_style.hpp"
#include "editor/editor_renderer.hpp"
#include "editor/editor_lua_wrapper.hpp"
#include "tileset_manager.hpp"
#include "window_manager.hpp"
#include "plugin_manager.hpp"
#include "map_manager.hpp"

#include <sol/sol.hpp>

using namespace arpiyi_editor;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

int main() {
    window_manager::init();
    tileset_manager::init();
    map_manager::init();
    editor::style::init();
    editor::lua_wrapper::init();
    plugin_manager::init();
    plugin_manager::load_plugins("data/plugins");

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

        editor::renderer::render();
        tileset_manager::render();
        map_manager::render();
        // ImGui::ShowDemoWindow(nullptr);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_manager::get_window());
    }

    ImGui::SaveIniSettingsToDisk("imgui.ini");
    glfwTerminate();
    return 0;
}
