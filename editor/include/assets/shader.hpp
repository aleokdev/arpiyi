#ifndef ARPIYI_SHADER_HPP
#define ARPIYI_SHADER_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "asset.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace arpiyi_editor::assets {

struct Shader {
    void destroy() {
        if (handle != -1){
            glDeleteProgram(handle);
            handle = -1;
        }
    }

    unsigned int handle = -1;
};

template<> struct LoadParams<Shader> {
    fs::path vert_path;
    fs::path frag_path;
};

unsigned int create_shader_stage(GLenum stage, fs::path path) {
    using namespace std::literals::string_literals;

    std::ifstream f(path);
    if (!f.good()) {
        throw std::runtime_error("Failed to open file: "s + path.generic_string());
    }
    std::string source((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    char* src = source.data();

    unsigned int shader = glCreateShader(stage);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infolog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infolog);
        throw std::runtime_error("Failed to compile shader:\n"s + source + "\nReason: "s + infolog);
    }

    return shader;
}

template<> inline void raw_load(Shader& shader, LoadParams<Shader> const& params) {
    using namespace std::literals::string_literals;

    unsigned int vtx = create_shader_stage(GL_VERTEX_SHADER, params.vert_path);
    unsigned int frag = create_shader_stage(GL_FRAGMENT_SHADER, params.frag_path);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vtx);
    glAttachShader(prog, frag);

    glLinkProgram(prog);
    int success;
    char infolog[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, nullptr, infolog);
        throw std::runtime_error("Failed to link shader.\nReason: "s + infolog);
    }

    glDeleteShader(vtx);
    glDeleteShader(frag);

    shader.handle = prog;
}

}

#endif // ARPIYI_SHADER_HPP
