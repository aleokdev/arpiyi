#include "tileset_manager.hpp"
#include "window_manager.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>
#include <noc_file_dialog.h>

#include <ass/ass_types.h>

namespace arpiyi_editor::tileset_manager {

Tileset current_tileset;
const char* tile_frag_shader_src = R"(
#version 430 core

in vec2 TexCoords;

layout(location = 0) uniform sampler2D tile;

out vec4 FragColor;

void main() {
    FragColor = texture(tile, TexCoords).rgba;
})";

const char* tile_vert_shader_src = R"(
#version 430 core

layout(location = 0) in vec2 iPos;

layout(location = 1) uniform mat4 model;
layout(location = 2) uniform mat4 projection;

out vec2 TexCoords;

void main() {
    TexCoords = iPos;

    gl_Position = projection * model * vec4(iPos, 0, 1);
})";
unsigned int tile_program;
glm::mat4 proj_mat;

void generate_splitted_quad(unsigned int* vao,
                            unsigned int* vbo,
                            std::size_t rows,
                            std::size_t columns) {
    // Format: {pos.x pos.y  ...}
    // UV and position data here is the same since position 0,0 is linked to UV 0,0 and
    // position 1,1 is linked to UV 1,1.
    // 2 because it's 2 position coords.
    constexpr auto sizeof_vertex = 2;
    constexpr auto sizeof_triangle = 3 * sizeof_vertex;
    constexpr auto sizeof_quad = 2 * sizeof_triangle;
    const auto sizeof_splitted_quad = rows * columns * sizeof_quad;

    float* result = new float[sizeof_splitted_quad];
    const float x_slice_size = 1.f / columns;
    const float y_slice_size = 1.f / rows;
    // Create a quad for each {x, y} position.
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < columns; x++) {
            const float min_x_pos = x * x_slice_size;
            const float min_y_pos = y * y_slice_size;
            const float max_x_pos = min_x_pos + x_slice_size;
            const float max_y_pos = min_y_pos + y_slice_size;

            // First triangle //
            /* X pos 1st vertex */ result[(x + y * columns) * sizeof_quad + 0] = min_x_pos;
            /* Y pos 1st vertex */ result[(x + y * columns) * sizeof_quad + 1] = min_y_pos;
            /* X pos 2nd vertex */ result[(x + y * columns) * sizeof_quad + 2] = max_x_pos;
            /* Y pos 2nd vertex */ result[(x + y * columns) * sizeof_quad + 3] = min_y_pos;
            /* X pos 3rd vertex */ result[(x + y * columns) * sizeof_quad + 4] = min_x_pos;
            /* Y pos 3rd vertex */ result[(x + y * columns) * sizeof_quad + 5] = max_y_pos;

            // Second triangle //
            /* X pos 1st vertex */ result[(x + y * columns) * sizeof_quad + 6] = max_x_pos;
            /* Y pos 1st vertex */ result[(x + y * columns) * sizeof_quad + 7] = min_y_pos;
            /* X pos 2nd vertex */ result[(x + y * columns) * sizeof_quad + 8] = max_x_pos;
            /* Y pos 2nd vertex */ result[(x + y * columns) * sizeof_quad + 9] = max_y_pos;
            /* X pos 3rd vertex */ result[(x + y * columns) * sizeof_quad + 10] = min_x_pos;
            /* Y pos 3rd vertex */ result[(x + y * columns) * sizeof_quad + 11] = max_y_pos;
        }
    }
    std::cout << "Last element: " << (((columns - 1) + (rows - 1) * columns) * sizeof_quad + 11)
              << std::endl;
    std::cout << "Size of splitted quad: " << sizeof_splitted_quad << std::endl;

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

    int fb_w, fb_h;
    glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);

    proj_mat = glm::ortho(0.0f, (float)fb_w, (float)fb_h, 0.0f);
}

void render() {
    if (!ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    static bool show_tile_preview = true;

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
                    auto& tex = *current_tileset.texture.get().operator->();
                    generate_splitted_quad(&current_tileset.vao, &current_tileset.vbo,
                                           tex.h / current_tileset.tile_size,
                                           tex.w / current_tileset.tile_size);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit" /*, current_tileset.texture.get()*/)) {
            static bool two_power = true;
            static int tile_size_slider = current_tileset.tile_size;
            static int last_tile_size = tile_size_slider;
            if (ImGui::SliderInt("Tile size", &tile_size_slider, 1, 256)) {
                if (two_power) {
                    tile_size_slider--;
                    tile_size_slider |= tile_size_slider >> 1;
                    tile_size_slider |= tile_size_slider >> 2;
                    tile_size_slider |= tile_size_slider >> 4;
                    tile_size_slider |= tile_size_slider >> 8;
                    tile_size_slider |= tile_size_slider >> 16;
                    tile_size_slider++;
                }
            } else if (last_tile_size != tile_size_slider) {
                current_tileset.tile_size = tile_size_slider;
                // Re-split the tileset if tilesize changed
                ImGui::SameLine();
                ImGui::TextDisabled("Recalculating quads...");
                if (current_tileset.vao != -1)
                    glDeleteBuffers(1, &current_tileset.vao);
                if (current_tileset.vbo != -1)
                    glDeleteBuffers(1, &current_tileset.vbo);
                auto& tex = *current_tileset.texture.get().operator->();
                generate_splitted_quad(&current_tileset.vao, &current_tileset.vbo,
                                       tex.h / current_tileset.tile_size,
                                       tex.w / current_tileset.tile_size);
                last_tile_size = current_tileset.tile_size;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Power of two", &two_power);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if(ImGui::MenuItem("Show Tile Preview", nullptr, show_tile_preview))
                show_tile_preview = !show_tile_preview;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (auto img = current_tileset.texture.get()) {
        ImVec2 tileset_render_pos = ImGui::GetCursorScreenPos();
        ImVec2 tileset_render_pos_max =
            ImVec2(tileset_render_pos.x + img->w, tileset_render_pos.y + img->h);

        const auto& io = ImGui::GetIO();
        // Substract half of the current tilesize to obtain a centered rect
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
        // Clip anything that is outside the tileset rect
        draw_list->PushClipRect(tileset_render_pos, tileset_render_pos_max, true);
        // Draw the tileset
        struct TilesetDrawData {
            ImVec2 tileset_render_pos;
            assets::Texture* tex;
        };
        draw_list->AddCallback(
            [](const ImDrawList* parent_list, const ImDrawCmd* cmd) {
                const TilesetDrawData* data = (TilesetDrawData*)cmd->UserCallbackData;
                glUseProgram(tile_program);
                constexpr int binding_index = 0;
                glBindVertexArray(current_tileset.vao);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, data->tex->handle);
                int fb_w, fb_h;
                glfwGetFramebufferSize(window_manager::get_window(), &fb_w, &fb_h);
                glScissor(0, 0, fb_w, fb_h);

                glm::mat4 model = glm::mat4(1);
                model = glm::translate(
                    model, glm::vec3{data->tileset_render_pos.x, data->tileset_render_pos.y, 0});
                model = glm::scale(model, glm::vec3{data->tex->w, data->tex->h, 1});
                std::cout << data->tileset_render_pos.x << std::endl;

                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0,
                             (data->tex->w / current_tileset.tile_size) *
                                 (data->tex->h / current_tileset.tile_size) * quad_verts);
                delete[] data;
            },
            new TilesetDrawData{tileset_render_pos, img.operator->()});
        draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

        // Draw the hovering rect
        draw_list->AddRect(
            mouse_pos,
            {mouse_pos.x + current_tileset.tile_size, mouse_pos.y + current_tileset.tile_size},
            0xFFFFFFFF, 0, ImDrawCornerFlags_All, 4.f);
        draw_list->AddRect(
            mouse_pos,
            {mouse_pos.x + current_tileset.tile_size, mouse_pos.y + current_tileset.tile_size},
            0xFF000000, 0, ImDrawCornerFlags_All, 2.f);
        draw_list->PopClipRect();
    } else
        ImGui::TextDisabled("No tileset loaded.");

    if(show_tile_preview)
    {
        if(auto img = current_tileset.texture.get(); ImGui::Begin("Tile Preview")){
            //ImGui::Image((ImTextureID)img->handle,ImVec2{current_tileset.tile_size * 3.f, current_tileset.tile_size * 3.f}, )
        }
        ImGui::End();
    }

    ImGui::End();
}

} // namespace arpiyi_editor::tileset_manager