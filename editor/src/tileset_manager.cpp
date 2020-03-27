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

std::vector<asset_manager::Handle<assets::Tileset>> tilesets;
asset_manager::Handle<assets::Tileset> current_tileset;
asset_manager::Handle<assets::Shader> tile_shader;
asset_manager::Handle<assets::Shader> grid_shader;
asset_manager::Handle<assets::Mesh> quad_mesh;
std::size_t tile_size = 48;

unsigned int grid_framebuffer;
assets::Texture grid_texture;

glm::mat4 proj_mat;

math::IVec2D tile_selection_start{0, 0};
math::IVec2D tile_selection_end{0, 0};

static void update_grid_texture() {
    auto tileset = current_tileset.get();
    if(!tileset) return;

    if (grid_texture.handle == -1)
        glDeleteTextures(1, &grid_texture.handle);
    glGenTextures(1, &grid_texture.handle);
    glBindTexture(GL_TEXTURE_2D, grid_texture.handle);

    const auto tileset_size = tileset->get_size_in_tiles(tile_size);
    grid_texture.w = tileset_size.x * tile_size;
    grid_texture.h = tileset_size.y * tile_size;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, grid_texture.w, grid_texture.h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, grid_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           grid_texture.handle, 0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw grid
    glUseProgram(grid_shader.get()->handle);
    glBindVertexArray(quad_mesh.get()->vao);
    glUniform4f(3, .9f, .9f, .9f, .5f);
    glUniform2ui(4, tileset_size.x, tileset_size.y);
    glViewport(0.f, 0.f, grid_texture.w, grid_texture.h);
    glm::mat4 model = glm::mat4(1);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, quad_verts);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init() {
    glGenFramebuffers(1, &grid_framebuffer);

    tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f);
}

void render() {
    if (!ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
    } else {
        if (auto ts = current_tileset.get()) {
            if (ImGui::BeginMenuBar()) {
                /*
                auto img = ts->texture.get();
                if (ImGui::BeginMenu("Edit", img)) {
                    static bool two_power = true;
                    static int tile_size_slider = ts->tile_size;
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
                }*/
                ImGui::EndMenuBar();
            }

            if (auto img = ts->texture.get()) {
                const ImVec2 tileset_render_pos = {
                    ImGui::GetCursorScreenPos().x,
                    ImGui::GetCursorScreenPos().y};
                const ImVec2 tileset_render_pos_max =
                    ImVec2(tileset_render_pos.x + img->w, tileset_render_pos.y + img->h);

                const auto& io = ImGui::GetIO();
                ImVec2 mouse_pos = io.MousePos;
                ImVec2 relative_mouse_pos =
                    ImVec2(mouse_pos.x - tileset_render_pos.x, mouse_pos.y - tileset_render_pos.y);
                // Snap the relative mouse position
                relative_mouse_pos.x =
                    (int)relative_mouse_pos.x - ((int)relative_mouse_pos.x % tile_size);
                relative_mouse_pos.y =
                    (int)relative_mouse_pos.y - ((int)relative_mouse_pos.y % tile_size);
                // Apply the same snapping effect to the regular mouse pos
                mouse_pos = ImVec2(relative_mouse_pos.x + tileset_render_pos.x,
                                   relative_mouse_pos.y + tileset_render_pos.y);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // Draw the tileset
                ImGui::Image(reinterpret_cast<ImTextureID>(img->handle), ImVec2{(float)img->w, (float)img->h});
                ImGui::SetCursorScreenPos(tileset_render_pos);
                ImGui::Image(reinterpret_cast<ImTextureID>(grid_texture.handle), ImVec2{(float)img->w, (float)img->h});

                // Clip anything that is outside the tileset rect
                draw_list->PushClipRect(tileset_render_pos, tileset_render_pos_max, true);

                const ImVec2 tile_selection_start_rel =
                    ImVec2{tileset_render_pos.x + (float)tile_selection_start.x * tile_size,
                           tileset_render_pos.y + (float)tile_selection_start.y * tile_size};
                const ImVec2 tile_selection_end_rel =
                    ImVec2{tileset_render_pos.x + (float)(tile_selection_end.x + 1) * tile_size,
                           tileset_render_pos.y + (float)(tile_selection_end.y + 1) * tile_size};
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

                    draw_list->AddRect(mouse_pos,
                                       {mouse_pos.x + tile_size, mouse_pos.y + tile_size},
                                       0xFFFFFFFF, 0, ImDrawCornerFlags_All, 4.f);
                    draw_list->AddRect(mouse_pos,
                                       {mouse_pos.x + tile_size, mouse_pos.y + tile_size},
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
                    const math::IVec2D tile_hovering{(int)(relative_mouse_pos.x / tile_size),
                                                     (int)(relative_mouse_pos.y / tile_size)};
                    const ImVec2 img_size{64, 64};
                    const math::IVec2D size_in_tiles = ts->get_size_in_tiles(tile_size);
                    const ImVec2 uv_min{(float)tile_hovering.x / (float)size_in_tiles.x,
                                        (float)tile_hovering.y / (float)size_in_tiles.y};
                    const ImVec2 uv_max{(float)(tile_hovering.x + 1) / (float)size_in_tiles.x,
                                        (float)(tile_hovering.y + 1) / (float)size_in_tiles.y};
                    ImGui::Image((ImTextureID)img->handle, img_size, uv_min, uv_max,
                                 ImVec4(1, 1, 1, tooltip_alpha));
                    ImGui::SameLine();
                    static std::size_t tile_id;
                    if (update_tooltip_info)
                        tile_id =
                            tile_hovering.x + tile_hovering.y * ts->get_size_in_tiles(tile_size).x;
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
                        tile_selection_start = {(int)(relative_mouse_pos.x / tile_size),
                                                (int)(relative_mouse_pos.y / tile_size)};
                        tile_selection_end = tile_selection_start;
                    } else {
                        tile_selection_end = {std::max(tile_selection_start.x,
                                                       (int)(relative_mouse_pos.x / tile_size)),
                                              std::max(tile_selection_start.y,
                                                       (int)(relative_mouse_pos.y / tile_size))};
                    }
                }
                pressed_last_frame = io.MouseDown[ImGuiMouseButton_Left];

            } else
                ImGui::TextDisabled("No texture attached to tileset.");
        } else
            ImGui::TextDisabled("No tileset loaded.");

        ImGui::End();
    }

    if (!ImGui::Begin("Tileset List", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
    } else {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...")) {
                const char* compatible_file_formats =
                    "Any file\0*\0JPEG\0*jpg;*.jpeg\0PNG\0*png\0TGA\0*.tga\0BMP\0*.bmp\0PSD\0"
                    "*.psd\0GIF\0*.gif\0HDR\0*.hdr\0PIC\0*.pic\0PNM\0*.pnm\0";
                const char* path_selected = noc_file_dialog_open(
                    NOC_FILE_DIALOG_OPEN, compatible_file_formats, nullptr, nullptr);
                if (path_selected && fs::is_regular_file(path_selected)) {
                    assets::Tileset tileset;
                    tileset.texture = asset_manager::load<assets::Texture>({path_selected, false});
                    tileset.name = fs::path(path_selected).filename();
                    current_tileset = tilesets.emplace_back(asset_manager::put(tileset));
                    update_grid_texture();
                }
            }
            ImGui::EndMenuBar();
        }
        if (tilesets.empty())
            ImGui::TextDisabled("No tilesets");
        else
            for (auto& _t : tilesets) {
                if (auto tileset = _t.get()) {
                    ImGui::TextDisabled("%zu", _t.get_id());
                    ImGui::SameLine();
                    if (ImGui::Selectable(tileset->name.c_str(), _t == current_tileset)) {
                        current_tileset = _t;
                        update_grid_texture();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, 0.f, 0.f, 1.f});
                    ImGui::TextUnformatted("Empty reference");
                    ImGui::PopStyleColor(1);
                }
            }
        ImGui::End();
    }
}

std::size_t get_tile_size() { return tile_size; }

std::vector<asset_manager::Handle<assets::Tileset>>& get_tilesets() { return tilesets; }

} // namespace arpiyi_editor::tileset_manager