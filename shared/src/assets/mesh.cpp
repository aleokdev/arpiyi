#include "assets/mesh.hpp"

#include <vector>

namespace arpiyi::assets {

Mesh Mesh::generate_quad() {
    // TODO: Remove and use MeshBuilder
    // Format: {pos.x pos.y uv.x uv.y ...}
    // 2 because it's 2 position coords and 2 UV coords.
    constexpr auto sizeof_vertex = 4;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;

    std::vector<float> result(sizeof_quad);
    constexpr float min_x_pos = 0;
    constexpr float min_y_pos = 0;
    constexpr float max_x_pos = 1;
    constexpr float max_y_pos = 1;
    // First triangle //
    /* X pos 1st vertex */ result[0] = min_x_pos;
    /* Y pos 1st vertex */ result[1] = min_y_pos;
    /* X UV 1st vertex  */ result[2] = min_x_pos;
    /* Y UV 1st vertex  */ result[3] = min_y_pos;
    /* X pos 2nd vertex */ result[4] = max_x_pos;
    /* Y pos 2nd vertex */ result[5] = min_y_pos;
    /* X UV 2nd vertex  */ result[6] = max_x_pos;
    /* Y UV 2nd vertex  */ result[7] = min_y_pos;
    /* X pos 3rd vertex */ result[8] = min_x_pos;
    /* Y pos 3rd vertex */ result[9] = max_y_pos;
    /* X UV 2nd vertex  */ result[10] = min_x_pos;
    /* Y UV 2nd vertex  */ result[11] = max_y_pos;

    // Second triangle //
    /* X pos 1st vertex */ result[12] = max_x_pos;
    /* Y pos 1st vertex */ result[13] = min_y_pos;
    /* X UV 1st vertex  */ result[14] = max_x_pos;
    /* Y UV 1st vertex  */ result[15] = min_y_pos;
    /* X pos 2nd vertex */ result[16] = max_x_pos;
    /* Y pos 2nd vertex */ result[17] = max_y_pos;
    /* X UV 2nd vertex  */ result[18] = max_x_pos;
    /* Y UV 2nd vertex  */ result[19] = max_y_pos;
    /* X pos 3rd vertex */ result[20] = min_x_pos;
    /* Y pos 3rd vertex */ result[21] = max_y_pos;
    /* X UV 3rd vertex  */ result[22] = min_x_pos;
    /* Y UV 3rd vertex  */ result[23] = max_y_pos;

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_quad * sizeof(float), result.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    // Vertex Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(0, 0);
    // UV Positions
    glEnableVertexAttribArray(1); // location 1
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glBindVertexBuffer(1, vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(1, 1);

    return Mesh{vao, vbo, 6};
}

}