#include "assets/mesh.hpp"

#include <vector>

namespace arpiyi_editor::assets {

Mesh Mesh::generate_split_quad(std::size_t x_slices,
                            std::size_t y_slices) {
    // Format: {pos.x pos.y  ...}
    // UV and position data here is the same since position 0,0 is linked to UV 0,0 and
    // position 1,1 is linked to UV 1,1.
    // 2 because it's 2 position coords.
    constexpr auto sizeof_vertex = 2;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = y_slices * x_slices * sizeof_quad;

    std::vector<float> result(sizeof_splitted_quad);
    const float x_slice_size = 1.f / x_slices;
    const float y_slice_size = 1.f / y_slices;
    // Create a quad for each {x, y} position.
    for (int y = 0; y < y_slices; y++) {
        for (int x = 0; x < x_slices; x++) {
            const float min_x_pos = x * x_slice_size;
            const float min_y_pos = y * y_slice_size;
            const float max_x_pos = min_x_pos + x_slice_size;
            const float max_y_pos = min_y_pos + y_slice_size;

            // First triangle //
            /* X pos 1st vertex */ result[(x + y * x_slices) * sizeof_quad + 0] = min_x_pos;
            /* Y pos 1st vertex */ result[(x + y * x_slices) * sizeof_quad + 1] = min_y_pos;
            /* X pos 2nd vertex */ result[(x + y * x_slices) * sizeof_quad + 2] = max_x_pos;
            /* Y pos 2nd vertex */ result[(x + y * x_slices) * sizeof_quad + 3] = min_y_pos;
            /* X pos 3rd vertex */ result[(x + y * x_slices) * sizeof_quad + 4] = min_x_pos;
            /* Y pos 3rd vertex */ result[(x + y * x_slices) * sizeof_quad + 5] = max_y_pos;

            // Second triangle //
            /* X pos 1st vertex */ result[(x + y * x_slices) * sizeof_quad + 6] = max_x_pos;
            /* Y pos 1st vertex */ result[(x + y * x_slices) * sizeof_quad + 7] = min_y_pos;
            /* X pos 2nd vertex */ result[(x + y * x_slices) * sizeof_quad + 8] = max_x_pos;
            /* Y pos 2nd vertex */ result[(x + y * x_slices) * sizeof_quad + 9] = max_y_pos;
            /* X pos 3rd vertex */ result[(x + y * x_slices) * sizeof_quad + 10] = min_x_pos;
            /* Y pos 3rd vertex */ result[(x + y * x_slices) * sizeof_quad + 11] = max_y_pos;
        }
    }

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_splitted_quad * sizeof(float), result.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    // Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, vbo, 0, 2 * sizeof(float));
    glVertexAttribBinding(0, 0);

    return Mesh{vao, vbo};
}

Mesh Mesh::generate_wrapping_split_quad(std::size_t x_slices,
                                            std::size_t y_slices,
                                            std::size_t max_quads_per_row) {
    // Format: {pos.x pos.y uv.x uv.y ...}
    // UV and position data here are NOT the same since position 1,0 *may* not be linked to UV 1,0
    // because it might have wrapped over.

    // 2 because it's 2 vertex coords and 2 UV coords.
    constexpr auto sizeof_vertex = 4;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = y_slices * x_slices * sizeof_quad;

    std::vector<float> result(sizeof_splitted_quad);
    const float x_slice_size = 1.f / x_slices;
    const float y_slice_size = 1.f / y_slices;
    // Create a quad for each position.
    for (int i = 0; i < x_slices * y_slices; i++) {
        const std::size_t quad_x = i % max_quads_per_row;
        const std::size_t quad_y = i / max_quads_per_row;
        const float min_vertex_x_pos = (float)quad_x * x_slice_size;
        const float min_vertex_y_pos = (float)quad_y * y_slice_size;
        const float max_vertex_x_pos = min_vertex_x_pos + x_slice_size;
        const float max_vertex_y_pos = min_vertex_y_pos + y_slice_size;

        const float min_uv_x_pos = (float)(i % x_slices) * x_slice_size;
        const float min_uv_y_pos = (float)(i / x_slices) * y_slice_size;
        const float max_uv_x_pos = min_uv_x_pos + x_slice_size;
        const float max_uv_y_pos = min_uv_y_pos + y_slice_size;

        std::size_t quad_n = sizeof_quad * i;
        // First triangle //
        /* X pos 1st vertex */ result[quad_n + 0] = min_vertex_x_pos;
        /* Y pos 1st vertex */ result[quad_n + 1] = min_vertex_y_pos;
        /* X UV 1st vertex  */ result[quad_n + 2] = min_uv_x_pos;
        /* Y UV 1st vertex  */ result[quad_n + 3] = min_uv_y_pos;
        /* X pos 2nd vertex */ result[quad_n + 4] = max_vertex_x_pos;
        /* Y pos 2nd vertex */ result[quad_n + 5] = min_vertex_y_pos;
        /* X UV 2nd vertex  */ result[quad_n + 6] = max_uv_x_pos;
        /* Y UV 2nd vertex  */ result[quad_n + 7] = min_uv_y_pos;
        /* X pos 3rd vertex */ result[quad_n + 8] = min_vertex_x_pos;
        /* Y pos 3rd vertex */ result[quad_n + 9] = max_vertex_y_pos;
        /* X UV 2nd vertex  */ result[quad_n + 10] = min_uv_x_pos;
        /* Y UV 2nd vertex  */ result[quad_n + 11] = max_uv_y_pos;

        // Second triangle //
        /* X pos 1st vertex */ result[quad_n + 12] = max_vertex_x_pos;
        /* Y pos 1st vertex */ result[quad_n + 13] = min_vertex_y_pos;
        /* X UV 1st vertex  */ result[quad_n + 14] = max_uv_x_pos;
        /* Y UV 1st vertex  */ result[quad_n + 15] = min_uv_y_pos;
        /* X pos 2nd vertex */ result[quad_n + 16] = max_vertex_x_pos;
        /* Y pos 2nd vertex */ result[quad_n + 17] = max_vertex_y_pos;
        /* X UV 2nd vertex  */ result[quad_n + 18] = max_uv_x_pos;
        /* Y UV 2nd vertex  */ result[quad_n + 19] = max_uv_y_pos;
        /* X pos 3rd vertex */ result[quad_n + 20] = min_vertex_x_pos;
        /* Y pos 3rd vertex */ result[quad_n + 21] = max_vertex_y_pos;
        /* X UV 3rd vertex  */ result[quad_n + 22] = min_uv_x_pos;
        /* Y UV 3rd vertex  */ result[quad_n + 23] = max_uv_y_pos;
    }

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_splitted_quad * sizeof(float), result.data(),
                 GL_STATIC_DRAW);

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

    return Mesh{vao, vbo};
}

Mesh Mesh::generate_quad() {
    // Format: {pos.x pos.y  ...}
    // UV and position data here is the same since position 0,0 is linked to UV 0,0 and
    // position 1,1 is linked to UV 1,1.
    // 2 because it's 2 position coords.
    constexpr auto sizeof_vertex = 2;
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
    /* X pos 2nd vertex */ result[2] = max_x_pos;
    /* Y pos 2nd vertex */ result[3] = min_y_pos;
    /* X pos 3rd vertex */ result[4] = min_x_pos;
    /* Y pos 3rd vertex */ result[5] = max_y_pos;

    // Second triangle //
    /* X pos 1st vertex */ result[6] = max_x_pos;
    /* Y pos 1st vertex */ result[7] = min_y_pos;
    /* X pos 2nd vertex */ result[8] = max_x_pos;
    /* Y pos 2nd vertex */ result[9] = max_y_pos;
    /* X pos 3rd vertex */ result[10] = min_x_pos;
    /* Y pos 3rd vertex */ result[11] = max_y_pos;

    unsigned int vao, vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_quad * sizeof(float), result.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    // Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, vbo, 0, 2 * sizeof(float));
    glVertexAttribBinding(0, 0);

    return Mesh{vao, vbo};
}

}