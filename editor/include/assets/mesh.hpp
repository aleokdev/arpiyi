#ifndef ARPIYI_MESH_HPP
#define ARPIYI_MESH_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "asset.hpp"

#include <cstdint>

namespace arpiyi_editor::assets {

struct Mesh {
    void destroy() {
        if (vao != -1){
            glDeleteVertexArrays(1, &vao);
            vao = -1;
        }
        if (vbo != -1){
            glDeleteBuffers(1, &vbo);
            vbo = -1;
        }
    }

    static Mesh generate_quad();

    static Mesh generate_split_quad(std::size_t x_slices,
                                       std::size_t y_slices);

    static Mesh generate_wrapping_split_quad(std::size_t x_slices,
                                                std::size_t y_slices,
                                                std::size_t max_quads_per_row);

    // TODO: Use global VAO
    unsigned int vao = -1;
    unsigned int vbo = -1;
};

} // namespace arpiyi_editor::assets

#endif // ARPIYI_MESH_HPP
