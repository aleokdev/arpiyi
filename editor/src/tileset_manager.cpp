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

#include "assets/shader.hpp"

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
        if(auto mesh = current_tileset.display_mesh.get()) {
            mesh->destroy();
            (*mesh) = assets::Mesh::generate_split_quad(slices.x, slices.y);
        } else {
            current_tileset.display_mesh = asset_manager::put(assets::Mesh::generate_split_quad(slices.x, slices.y));
        }
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
                glBindVertexArray(current_tileset.display_mesh.get()->vao);
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