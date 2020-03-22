#include "tileset_manager.hpp"
#include "window_manager.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>
#include <noc_file_dialog.h>
#include <vector>

#include <util/math.hpp>

namespace arpiyi_editor::tileset_manager {

assets::Tileset current_tileset;
/* clang-format off */
const char* tile_frag_shader_src =
    #include "shaders/tile.frag"

const char* tile_vert_shader_src =
    #include "shaders/tile.vert"

        /* clang-format on */
    unsigned int tile_program;
glm::mat4 proj_mat;

math::IVec2D tile_selection_start{0, 0};
math::IVec2D tile_selection_end{0, 0};

void generate_splitted_quad(unsigned int* vao,
                            unsigned int* vbo,
                            std::size_t x_slices,
                            std::size_t y_slices) {
    // Format: {pos.x pos.y  ...}
    // UV and position data here is the same since position 0,0 is linked to UV 0,0 and
    // position 1,1 is linked to UV 1,1.
    // 2 because it's 2 position coords.
    constexpr auto sizeof_vertex = 2;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = y_slices * x_slices * sizeof_quad;

    float* result = new float[sizeof_splitted_quad];
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

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_splitted_quad * sizeof(float), result, GL_STATIC_DRAW);

    glBindVertexArray(*vao);
    // Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, *vbo, 0, 2 * sizeof(float));
    glVertexAttribBinding(0, 0);

    delete[] result;
}

void generate_wrapping_splitted_quad(unsigned int* vao,
                                     unsigned int* vbo,
                                     std::size_t x_slices,
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

    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);

    // Fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof_splitted_quad * sizeof(float), result.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(*vao);
    // Vertex Positions
    glEnableVertexAttribArray(0); // location 0
    glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
    glBindVertexBuffer(0, *vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(0, 0);
    // UV Positions
    glEnableVertexAttribArray(1); // location 1
    glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glBindVertexBuffer(1, *vbo, 0, 4 * sizeof(float));
    glVertexAttribBinding(1, 1);

    glBindVertexArray(0);
}

void init() {
    unsigned int tile_vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(tile_vert_shader, 1, &tile_vert_shader_src, NULL);
    glCompileShader(tile_vert_shader);

    int success;
    char infoLog[512];
    glGetShaderiv(tile_vert_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(tile_vert_shader, 512, NULL, infoLog);
        std::cerr << "Vertex shader COMPILATION FAILED:\n" << infoLog << std::endl;
    }

    unsigned int tile_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(tile_frag_shader, 1, &tile_frag_shader_src, NULL);
    glCompileShader(tile_frag_shader);

    glGetShaderiv(tile_frag_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(tile_frag_shader, 512, NULL, infoLog);
        std::cerr << "Fragment shader COMPILATION FAILED:\n" << infoLog << std::endl;
    }

    tile_program = glCreateProgram();
    glAttachShader(tile_program, tile_vert_shader);
    glAttachShader(tile_program, tile_frag_shader);
    glLinkProgram(tile_program);

    glGetProgramiv(tile_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(tile_program, 512, NULL, infoLog);
        std::cerr << "Tile program LINKING FAILED:\n" << infoLog << std::endl;
    }

    glDeleteShader(tile_vert_shader);
    glDeleteShader(tile_frag_shader);
}

void render() {
    auto fb_size = window_manager::get_framebuf_size();
    proj_mat = glm::ortho(0.0f, (float)fb_size.x, (float)fb_size.y, 0.0f);

    if (!ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }
    const auto& update_tileset_quads = [&]() {
        math::IVec2D slices = current_tileset.get_size_in_tiles();
        generate_splitted_quad(&current_tileset.vao, &current_tileset.vbo, slices.x, slices.y);
    };

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load from image...")) {
                const char* compatible_file_formats =
                    "Any file\0*\0JPEG\0*jpg;*.jpeg\0PNG\0*png\0TGA\0*.tga\0BMP\0*.bmp\0PSD\0"
                    "*.psd\0GIF\0*.gif\0HDR\0*.hdr\0PIC\0*.pic\0PNM\0*.pnm\0";
                std::string_view path_selected = noc_file_dialog_open(
                    NOC_FILE_DIALOG_OPEN, compatible_file_formats, nullptr, nullptr);
                if (path_selected.data() && fs::is_regular_file(path_selected)) {
                    if (current_tileset.texture.get())
                        current_tileset.texture.unload();
                    current_tileset.texture =
                        asset_manager::load<assets::Texture>({path_selected, false});
                    if (current_tileset.vao != -1)
                        glDeleteBuffers(1, &current_tileset.vao);
                    if (current_tileset.vbo != -1)
                        glDeleteBuffers(1, &current_tileset.vbo);
                    update_tileset_quads();
                }
            }
            ImGui::EndMenu();
        }
        auto img = current_tileset.texture.get();
        if (ImGui::BeginMenu("Edit", img)) {
            static bool two_power = true;
            static int tile_size_slider = current_tileset.tile_size;
            static int last_tile_size = tile_size_slider;
            if (ImGui::SliderInt("Tile size", &tile_size_slider, 8, 256)) {
                if (two_power) {
                    tile_size_slider--;
                    tile_size_slider |= tile_size_slider >> 1;
                    tile_size_slider |= tile_size_slider >> 2;
                    tile_size_slider |= tile_size_slider >> 4;
                    tile_size_slider |= tile_size_slider >> 8;
                    tile_size_slider |= tile_size_slider >> 16;
                    tile_size_slider++;
                }
            }
            if (last_tile_size != tile_size_slider) {
                current_tileset.tile_size = tile_size_slider;
                // Re-split the tileset if tilesize changed
                ImGui::SameLine();
                if (current_tileset.vao != -1)
                    glDeleteBuffers(1, &current_tileset.vao);
                if (current_tileset.vbo != -1)
                    glDeleteBuffers(1, &current_tileset.vbo);
                auto& tex = *current_tileset.texture.get().operator->();
                update_tileset_quads();
            }

            ImGui::SameLine();
            ImGui::Checkbox("Power of two", &two_power);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (auto img = current_tileset.texture.get()) {
        const ImVec2 tileset_render_pos = {
            ImGui::GetCursorScreenPos().x,
            ImGui::GetCursorScreenPos().y + 10}; // Offset by 10 in the Y axis to prevent clipping
        const ImVec2 tileset_render_pos_max =
            ImVec2(tileset_render_pos.x + img->w, tileset_render_pos.y + img->h);

        const auto& io = ImGui::GetIO();
        ImVec2 mouse_pos = io.MousePos;
        ImVec2 relative_mouse_pos =
            ImVec2(mouse_pos.x - tileset_render_pos.x, mouse_pos.y - tileset_render_pos.y);
        // Snap the relative mouse position
        relative_mouse_pos.x =
            (int)relative_mouse_pos.x - ((int)relative_mouse_pos.x % current_tileset.tile_size);
        relative_mouse_pos.y =
            (int)relative_mouse_pos.y - ((int)relative_mouse_pos.y % current_tileset.tile_size);
        // Apply the same snapping effect to the regular mouse pos
        mouse_pos = ImVec2(relative_mouse_pos.x + tileset_render_pos.x,
                           relative_mouse_pos.y + tileset_render_pos.y);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 content_region_avail = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##_tileset", ImVec2{(float)img->w, (float)img->h});

        // Clip anything that is outside the tileset rect
        draw_list->PushClipRect(tileset_render_pos, tileset_render_pos_max, true);

        // Draw the tileset
        struct TilesetDrawData {
            ImVec2 tileset_render_pos;
            ImVec2 window_content_size;
            assets::Texture* tex;
        };
        draw_list->AddCallback(
            [](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
                const TilesetDrawData* data = (TilesetDrawData*)cmd->UserCallbackData;
                glUseProgram(tile_program);
                glBindVertexArray(current_tileset.vao);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, data->tex->handle);
                int fb_w, fb_h;
                glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
                glScissor(data->tileset_render_pos.x, data->window_content_size.y - fb_h,
                          data->window_content_size.x + data->tileset_render_pos.x, fb_h);

                glm::mat4 model = glm::mat4(1);
                model = glm::translate(
                    model, glm::vec3{data->tileset_render_pos.x, data->tileset_render_pos.y, 0});
                model = glm::scale(model, glm::vec3{data->tex->w, data->tex->h, 1});

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0,
                             (data->tex->w / current_tileset.tile_size) *
                                 (data->tex->h / current_tileset.tile_size) * quad_verts);
                delete[] data;
            },
            new TilesetDrawData{tileset_render_pos, content_region_avail, img.operator->()});
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

        const ImVec2 tile_selection_start_rel = ImVec2{
            tileset_render_pos.x + (float)tile_selection_start.x * current_tileset.tile_size,
            tileset_render_pos.y + (float)tile_selection_start.y * current_tileset.tile_size};
        const ImVec2 tile_selection_end_rel = ImVec2{
            tileset_render_pos.x + (float)(tile_selection_end.x + 1) * current_tileset.tile_size,
            tileset_render_pos.y + (float)(tile_selection_end.y + 1) * current_tileset.tile_size};
        // Draw the selected rect
        draw_list->AddRect(tile_selection_start_rel, tile_selection_end_rel, 0xFFFFFFFF, 0,
                           ImDrawCornerFlags_All, 5.f);
        draw_list->AddRect(tile_selection_start_rel, tile_selection_end_rel, 0xFF000000, 0,
                           ImDrawCornerFlags_All, 2.f);

        // Draw the hovering rect and check if the preview tooltip should appear or not
        ImGui::SetNextWindowPos(io.MousePos);
        static float tooltip_alpha = 0;
        bool update_tooltip_info;
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) &&
            ImGui::IsMouseHoveringRect(tileset_render_pos, tileset_render_pos_max)) {
            update_tooltip_info = true;
            tooltip_alpha += (1.f - tooltip_alpha) / 16.f;

            draw_list->AddRect(
                mouse_pos,
                {mouse_pos.x + current_tileset.tile_size, mouse_pos.y + current_tileset.tile_size},
                0xFFFFFFFF, 0, ImDrawCornerFlags_All, 4.f);
            draw_list->AddRect(
                mouse_pos,
                {mouse_pos.x + current_tileset.tile_size, mouse_pos.y + current_tileset.tile_size},
                0xFF000000, 0, ImDrawCornerFlags_All, 2.f);

            draw_list->PopClipRect();
        } else {
            update_tooltip_info = false;
            tooltip_alpha += (0.f - tooltip_alpha) / 4.f;
            draw_list->PopClipRect();
        }

        ImGui::SetNextWindowBgAlpha(tooltip_alpha);
        // Draw preview of current tile being hovered
        ImGui::BeginTooltip();
        {
            const math::IVec2D tile_hovering{
                (int)(relative_mouse_pos.x / current_tileset.tile_size),
                (int)(relative_mouse_pos.y / current_tileset.tile_size)};
            const ImVec2 img_size{64, 64};
            const math::IVec2D size_in_tiles = current_tileset.get_size_in_tiles();
            const ImVec2 uv_min{(float)tile_hovering.x / (float)size_in_tiles.x,
                                (float)tile_hovering.y / (float)size_in_tiles.y};
            const ImVec2 uv_max{(float)(tile_hovering.x + 1) / (float)size_in_tiles.x,
                                (float)(tile_hovering.y + 1) / (float)size_in_tiles.y};
            ImGui::Image((ImTextureID)img->handle, img_size, uv_min, uv_max,
                         ImVec4(1, 1, 1, tooltip_alpha));
            ImGui::SameLine();
            static std::size_t tile_id;
            if (update_tooltip_info)
                tile_id = tile_hovering.x + tile_hovering.y * current_tileset.get_size_in_tiles().x;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, .8f, .8f, tooltip_alpha});
            ImGui::Text("ID %zu", tile_id);
            ImGui::Text("UV coords: {%.2f~%.2f, %.2f~%.2f}", uv_min.x, uv_max.x, uv_min.y,
                        uv_max.y);
            ImGui::PopStyleColor(1);
        }
        ImGui::EndTooltip();

        static bool pressed_last_frame = false;
        if (io.MouseDown[ImGuiMouseButton_Left]) {
            if (!pressed_last_frame) {
                tile_selection_start = {(int)(relative_mouse_pos.x / current_tileset.tile_size),
                                        (int)(relative_mouse_pos.y / current_tileset.tile_size)};
                tile_selection_end = tile_selection_start;
            } else {
                tile_selection_end = {
                    std::max(tile_selection_start.x,
                             (int)(relative_mouse_pos.x / current_tileset.tile_size)),
                    std::max(tile_selection_start.y,
                             (int)(relative_mouse_pos.y / current_tileset.tile_size))};
            }
        }
        pressed_last_frame = io.MouseDown[ImGuiMouseButton_Left];

    } else
        ImGui::TextDisabled("No tileset loaded.");

    ImGui::End();
}

} // namespace arpiyi_editor::tileset_manager