#include "window_manager.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace arpiyi_editor::window_manager {

GLFWwindow* window;

void init() {
    if (!glfwInit()) {
        std::cerr << "Couldn't init GLFW." << std::endl;
        return;
    }

    // Use OpenGL 4.5
    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    window = glfwCreateWindow(1080, 720, "Arpiyi Editor", nullptr, nullptr);
    if (!window) {
        std::cerr << "Couldn't create window." << std::endl;
        return;
    }
    // Activate VSync and fix FPS
    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEBUG_OUTPUT);
    glCullFace(GL_FRONT_AND_BACK);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

GLFWwindow* get_window() {
    return window;
}

glm::mat4 get_projection() {
    int fb_w, fb_h;
    glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
    return glm::ortho(0.f, (float)fb_w, 0.f, (float)fb_h);
}

math::IVec2D get_framebuf_size() {
    int fb_w, fb_h;
    glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
    return {fb_w, fb_h};
}

}