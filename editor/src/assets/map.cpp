#include "assets/map.hpp"

namespace arpiyi_editor::assets {

Mesh Map::Layer::generate_layer_split_quad() {
    // Format: {pos.x pos.y uv.x uv.y ...}
    // 2 because it's 2 position coords and 2 UV coords.
    constexpr auto sizeof_vertex = 4;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = height * width * sizeof_quad;

    std::vector<float> result(sizeof_splitted_quad);
    const float x_slice_size = 1.f / width;
    const float y_slice_size = 1.f / height;
    assert(tileset.get());
    const auto& tl = *tileset.get();
    // Create a quad for each {x, y} position.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const float min_vertex_x_pos = static_cast<float>(x) * x_slice_size;
            const float min_vertex_y_pos = static_cast<float>((int)height - y - 1) * y_slice_size;
            const float max_vertex_x_pos = min_vertex_x_pos + x_slice_size;
            const float max_vertex_y_pos = min_vertex_y_pos + y_slice_size;

            const math::Rect2D uv_pos = tl.get_uv(get_tile({x, y}).id);

            const auto quad_n = (x + y * width) * sizeof_quad;
            // First triangle //
            /* X pos 1st vertex */ result[quad_n + 0] = min_vertex_x_pos;
            /* Y pos 1st vertex */ result[quad_n + 1] = min_vertex_y_pos;
            /* X UV 1st vertex  */ result[quad_n + 2] = uv_pos.start.x;
            /* Y UV 1st vertex  */ result[quad_n + 3] = uv_pos.start.y;
            /* X pos 2nd vertex */ result[quad_n + 4] = max_vertex_x_pos;
            /* Y pos 2nd vertex */ result[quad_n + 5] = min_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 6] = uv_pos.end.x;
            /* Y UV 2nd vertex  */ result[quad_n + 7] = uv_pos.start.y;
            /* X pos 3rd vertex */ result[quad_n + 8] = min_vertex_x_pos;
            /* Y pos 3rd vertex */ result[quad_n + 9] = max_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 10] = uv_pos.start.x;
            /* Y UV 2nd vertex  */ result[quad_n + 11] = uv_pos.end.y;

            // Second triangle //
            /* X pos 1st vertex */ result[quad_n + 12] = max_vertex_x_pos;
            /* Y pos 1st vertex */ result[quad_n + 13] = min_vertex_y_pos;
            /* X UV 1st vertex  */ result[quad_n + 14] = uv_pos.end.x;
            /* Y UV 1st vertex  */ result[quad_n + 15] = uv_pos.start.y;
            /* X pos 2nd vertex */ result[quad_n + 16] = max_vertex_x_pos;
            /* Y pos 2nd vertex */ result[quad_n + 17] = max_vertex_y_pos;
            /* X UV 2nd vertex  */ result[quad_n + 18] = uv_pos.end.x;
            /* Y UV 2nd vertex  */ result[quad_n + 19] = uv_pos.end.y;
            /* X pos 3rd vertex */ result[quad_n + 20] = min_vertex_x_pos;
            /* Y pos 3rd vertex */ result[quad_n + 21] = max_vertex_y_pos;
            /* X UV 3rd vertex  */ result[quad_n + 22] = uv_pos.start.x;
            /* Y UV 3rd vertex  */ result[quad_n + 23] = uv_pos.end.y;
        }
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

Map::Layer::Layer(i64 width, i64 height, asset_manager::Handle<assets::Tileset> t) :
    tileset(t), width(width), height(height), tiles(width * height) {
    mesh = asset_manager::put(generate_layer_split_quad());
}

} // namespace arpiyi_editor::assets