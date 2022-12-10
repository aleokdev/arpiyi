#ifndef ARPIYI_MESH_HPP
#define ARPIYI_MESH_HPP

/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */

#include "asset.hpp"

#include <cstdint>

#include "util/intdef.hpp"

namespace arpiyi::assets {

struct Mesh {
    static Mesh generate_quad();

    static Mesh generate_split_quad(u32 x_slices,
                                       u32 y_slices);

    static Mesh generate_wrapping_split_quad(u32 x_slices,
                                             u32 y_slices,
                                             u32 max_quads_per_row);

    // TODO: Use global VAO
    unsigned int vao = -1;
    unsigned int vbo = -1;
    i32 vertex_count = 0;
    constexpr static auto noobj = static_cast<decltype(vao)>(-1);
};

template<> inline void raw_unload(Mesh& mesh) {
    if (mesh.vao != Mesh::noobj){
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = Mesh::noobj;
    }
    if (mesh.vbo != Mesh::noobj){
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = Mesh::noobj;
    }
}

} // namespace arpiyi_editor::assets

#endif // ARPIYI_MESH_HPP
