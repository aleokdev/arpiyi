#ifndef ARPIYI_MESH_HPP
#define ARPIYI_MESH_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "asset.hpp"

#include <cstdint>

#include "util/intdef.hpp"

namespace arpiyi_editor::assets {

struct Mesh {
    void destroy() {
        if (vao != noobj){
            glDeleteVertexArrays(1, &vao);
            vao = noobj;
        }
        if (vbo != noobj){
            glDeleteBuffers(1, &vbo);
            vbo = noobj;
        }
    }

    static Mesh generate_quad();

    static Mesh generate_split_quad(u32 x_slices,
                                       u32 y_slices);

    static Mesh generate_wrapping_split_quad(u32 x_slices,
                                             u32 y_slices,
                                             u32 max_quads_per_row);

    // TODO: Use global VAO
    unsigned int vao = -1;
    unsigned int vbo = -1;
    constexpr static auto noobj = static_cast<decltype(vao)>(-1);
};

} // namespace arpiyi_editor::assets

#endif // ARPIYI_MESH_HPP
