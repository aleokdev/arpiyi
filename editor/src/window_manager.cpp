#include "window_manager.hpp"

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "util/defs.hpp"
#include <anton/math/transform.hpp>
#include <iostream>

namespace arpiyi::window_manager {

static GLFWwindow* window;
static std::unique_ptr<renderer::Renderer> renderer;

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

    // Do not send bloat information
    if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    std::cout << "[" << stringify_severity(severity) << ":" << stringify_type(type) << " in "
              << stringify_source(source) << "]: " << message << std::endl;
}

bool init() {
    if (!glfwInit()) {
        std::cerr << "Couldn't init GLFW." << std::endl;
        return false;
    }

    // Use OpenGL 4.5
    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    window = glfwCreateWindow(1080, 720, "Arpiyi Editor", nullptr, nullptr);
    if (!window) {
        std::cerr
            << "Couldn't create window. Check your GPU drivers, as arpiyi requires OpenGL 4.5."
            << std::endl;
        return false;
    }
    // Activate VSync and fix FPS
    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT_AND_BACK);

    glDebugMessageCallback(debug_callback, nullptr);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    renderer = std::make_unique<renderer::Renderer>(window);
    return true;
}

GLFWwindow* get_window() { return window; }
renderer::Renderer& get_renderer() { return *renderer; }

aml::Matrix4 get_projection() {
    int fb_w, fb_h;
    glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
    return aml::orthographic_rh(0.f, (float)fb_w, 0.f, (float)fb_h, -1000000.f, 1000000.f);
}

math::IVec2D get_framebuf_size() {
    int fb_w, fb_h;
    glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
    return {fb_w, fb_h};
}

} // namespace arpiyi::window_manager