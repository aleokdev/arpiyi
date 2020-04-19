#include "assets/entity.hpp"
#include "assets/map.hpp"
#include "assets/sprite.hpp"
#include "assets/texture.hpp"
#include "serializer.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>

#include "util/defs.hpp"

namespace fs = std::filesystem;
using namespace arpiyi;

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

    const auto callback = [](auto str, auto progress) {
        std::cout << "Loading " << str << "... (" << std::setprecision(0) << std::fixed
                  << (progress * 100) << "%)" << std::endl;
    };

    fs::path project_path = fs::absolute(argv[1]);
    serializer::load_assets<assets::Texture>(project_path, callback);
    serializer::load_assets<assets::Sprite>(project_path, callback);
    serializer::load_assets<assets::Entity>(project_path, callback);
    serializer::load_assets<assets::Map>(project_path, callback);
    std::cout << "Finished loading." << std::endl;

    while (!glfwWindowShouldClose(window)) { glfwPollEvents(); }

    glfwTerminate();

    return 0;
}