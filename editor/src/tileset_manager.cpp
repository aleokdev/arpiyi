#include "tileset_manager.hpp"
#include "window_manager.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <noc_file_dialog.h>
#include <vector>

#include <util/math.hpp>

#include "assets/shader.hpp"

namespace arpiyi_editor::tileset_manager {

std::vector<asset_manager::Handle<assets::Tileset>> tilesets;
asset_manager::Handle<assets::Shader> tile_shader;
asset_manager::Handle<assets::Shader> grid_shader;
asset_manager::Handle<assets::Shader> uv_tile_shader;
asset_manager::Handle<assets::Mesh> quad_mesh;
std::size_t tile_size = 48;

unsigned int grid_framebuffer;
assets::Texture grid_texture;

glm::mat4 proj_mat;

TilesetSelection selection{0, {0, 0}, {0, 0}};

static void update_grid_texture() {
    auto tileset = selection.tileset.get();
    if (!tileset)
        return;

    if (grid_texture.handle == assets::Texture::nohandle)
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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, grid_texture.handle,
                           0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw grid
    glUseProgram(grid_shader.get()->handle);
    glBindVertexArray(quad_mesh.get()->vao);
    glUniform4f(3, .9f, .9f, .9f, .5f); // Grid color
    glUniform2ui(4, tileset_size.x, tileset_size.y);
    glViewport(0.f, 0.f, grid_texture.w, grid_texture.h);
    glm::mat4 model = glm::mat4(1);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, quad_verts);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

asset_manager::Handle<assets::Texture>
calculate_rpgmaker_a12_auto_tileset_texture(assets::Texture const& source_tex) {
    // The format RPGMaker uses for auto-tiling is pretty simple.
    // It consists of 6 tiles (Each one composed by 2x2 minitiles):
    // -----------------------------------------
    // display_tile         all_external_corners
    // upper-left corner    upper-right corner
    // lower-left corner    lower-right corner
    // -----------------------------------------
    // display_tile is actually not used in the calculations; it's just the tile that RPGMaker shows
    // in the editor.
    // For more information, check out
    // https://blog.rpgmakerweb.com/tutorials/anatomy-of-an-autotile/

    const math::IVec2D rpgmaker_tileset_size_in_tiles{
        static_cast<i32>(source_tex.w) / static_cast<i32>(tile_size),
        static_cast<i32>(source_tex.h) / static_cast<i32>(tile_size)};
    glm::mat4 tex_mat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

    // 64 tiles are needed to cache all possible combinations of corners.
    // HOWEVER, some of them are equal. RPGMaker-style texture only uses corner textures when there
    // aren't sides next to them. By doing some math, we end with the conclusion that only *46*
    // tiles are needed. Since 46 can't be nicely split into a square/rectangle texture, we'll just
    // do a 1*tile_size x 46*tile_size texture.
    unsigned int temp_fb;
    glGenFramebuffers(1, &temp_fb);
    assets::Texture generated_texture;
    glGenTextures(1, &generated_texture.handle);
    glBindTexture(GL_TEXTURE_2D, generated_texture.handle);
    generated_texture.w = 8 * tile_size;
    generated_texture.h = 8 * tile_size;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 8 * tile_size, 8 * tile_size, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // Attach the generated texture to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, temp_fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           generated_texture.handle, 0);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up
    glUseProgram(uv_tile_shader.get()->handle);
    glBindVertexArray(quad_mesh.get()->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source_tex.handle);
    glViewport(0.f, 0.f, generated_texture.w, generated_texture.h);

    // Calculate each corner variation texture and draw it to the framebuffer.
    for (u8 corner_variation = 0; corner_variation < 0xFF; corner_variation++) {
        // For each bit of corner_variation, 1 is "tile continues in that corner/side", 0 is the
        // opposite.

        constexpr u8 minitile_count = 4;
        // Let's first go through each one of the minitiles in the corner variation.
        for (u8 minitile = 0; minitile < minitile_count; minitile++) {
            // "case" is used here to identify the type of minitile. Check out:
            // https://dat5n5oxdq-flywheel.netdna-ssl.com/wp-content/uploads/2012/01/AT-Organization.png
            // Or alternatively, the image at
            // https://blog.rpgmakerweb.com/tutorials/anatomy-of-an-autotile/ Which binds each
            // minitile to a number and letter.
            // A minitile can be represented with a letter and a number. The letters go from A to D
            // and the numbers go from 1 to 5. The letters represent the location of the minitile
            // relative to the parent tile; The numbers represent the corner variation of that
            // minitile. "A" minitiles represent those placed in the upper left part of the tile.
            // "B" minitiles represent those placed in the upper right part of the tile.
            // "C" minitiles represent those placed in the lower left part of the tile.
            // "D" minitiles represent those placed in the lower right part of the tile.
            // "1" minitiles represent those with two adjacent tiles (On both sides of the minitile)
            // "2" minitiles represent those with no adjacent tiles (On both sides of the minitile)
            // "3" minitiles represent those with every adjacent tile (On both sides of the minitile
            // & on the corner) "4" minitiles represent those with an adjacent tile in their left or
            // right, depending on the minitile. "5" minitiles represent those with an adjacent tile
            // above or below, depending on the minitile.

            // Represents the adjacent tiles for this minitile.
            bool has_corner;
            bool has_vert_side;
            bool has_horz_side;

            enum tile_sides {
                upper_left_corner = 1 << 7,
                upper_middle_side = 1 << 6,
                upper_right_corner = 1 << 5,
                middle_left_side = 1 << 4,
                middle_right_side = 1 << 3,
                lower_left_corner = 1 << 2,
                lower_middle_side = 1 << 1,
                lower_right_corner = 1 << 0
            };

            switch (minitile) {
                case 0: // Upper left minitile ("A" case)
                    has_corner = corner_variation & upper_left_corner;
                    has_vert_side = corner_variation & upper_middle_side;
                    has_horz_side = corner_variation & middle_left_side;
                    break;
                case 1: // Upper right minitile ("B" case)
                    has_corner = corner_variation & upper_right_corner;
                    has_vert_side = corner_variation & upper_middle_side;
                    has_horz_side = corner_variation & middle_right_side;
                    break;
                case 2: // Lower left minitile ("C" case)
                    has_corner = corner_variation & lower_left_corner;
                    has_vert_side = corner_variation & lower_middle_side;
                    has_horz_side = corner_variation & middle_left_side;
                    break;
                case 3: // Lower right minitile ("D" case)
                    has_corner = corner_variation & lower_right_corner;
                    has_vert_side = corner_variation & lower_middle_side;
                    has_horz_side = corner_variation & middle_right_side;
                    break;
            }

            constexpr float target_tile_uv_scale = 1.f / 8.f;
            const float source_tile_uv_size_x =
                            1.f / static_cast<float>(rpgmaker_tileset_size_in_tiles.x),
                        source_tile_uv_size_y =
                            1.f / static_cast<float>(rpgmaker_tileset_size_in_tiles.y);

            float source_minitile_relative_x, source_minitile_relative_y;
            if (!has_vert_side && !has_horz_side && has_corner) { // "1" case
                source_minitile_relative_x =
                    source_tile_uv_size_x / 2.f * static_cast<float>((minitile % 2) + 2);
                source_minitile_relative_y =
                    source_tile_uv_size_y / 2.f * static_cast<float>((minitile / 2));
            } else { // Any other case ("2" to "5")
                enum MinitileFacing { right = 0b01, down = 0b10 };

                float minitile_pos_x = static_cast<float>(
                          (minitile % 2) + (has_horz_side
                                                ? ((minitile & MinitileFacing::right) ? 2 : 0)
                                                : ((minitile & MinitileFacing::right) ? 0 : 2))),
                      minitile_pos_y = static_cast<float>(
                          (minitile / 2) + (has_vert_side
                                                ? ((minitile & MinitileFacing::down) ? 2 : 0)
                                                : ((minitile & MinitileFacing::down) ? 0 : 2)));

                source_minitile_relative_x = source_tile_uv_size_x / 2.f * minitile_pos_x;
                source_minitile_relative_y = source_tile_uv_size_y / 2.f * (minitile_pos_y + 2);
            }

            const float target_minitile_x =
                            target_tile_uv_scale * (static_cast<float>(corner_variation % 8) +
                                                    static_cast<float>(minitile % 2) / 2.f),
                        target_minitile_y =
                            target_tile_uv_scale * (static_cast<float>(corner_variation / 8) +
                                                    static_cast<float>(minitile / 2) / 2.f);
            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(target_minitile_x, target_minitile_y, 0));
            model = glm::scale(model, glm::vec3(target_tile_uv_scale / 2.f));
            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(tex_mat));
            glUniform2f(3, source_minitile_relative_x, source_minitile_relative_y); // UV start
            glUniform2f(4, source_minitile_relative_x + source_tile_uv_size_x / 2.f,
                        source_minitile_relative_y + source_tile_uv_size_y / 2.f); // UV end
            constexpr int quad_verts = 2 * 3;
            glDrawArrays(GL_TRIANGLES, 0, quad_verts);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteBuffers(1, &temp_fb);

    return asset_manager::put(generated_texture);
}

void init() {
    glGenFramebuffers(1, &grid_framebuffer);

    tile_shader = asset_manager::load<assets::Shader>({"data/basic.vert", "data/basic.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    uv_tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile_uv.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f);
}

void render() {
    if (ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (auto ts = selection.tileset.get()) {
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
                const ImVec2 tileset_render_pos = {ImGui::GetCursorScreenPos().x,
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
                ImGui::Image(reinterpret_cast<ImTextureID>(img->handle),
                             ImVec2{(float)img->w, (float)img->h});
                ImGui::SetCursorScreenPos(tileset_render_pos);
                ImGui::Image(reinterpret_cast<ImTextureID>(grid_texture.handle),
                             ImVec2{(float)img->w, (float)img->h});

                // Clip anything that is outside the tileset rect
                draw_list->PushClipRect(tileset_render_pos, tileset_render_pos_max, true);

                const ImVec2 tile_selection_start_rel =
                    ImVec2{tileset_render_pos.x + (float)selection.selection_start.x * tile_size,
                           tileset_render_pos.y + (float)selection.selection_start.y * tile_size};
                const ImVec2 tile_selection_end_rel = ImVec2{
                    tileset_render_pos.x + (float)(selection.selection_end.x + 1) * tile_size,
                    tileset_render_pos.y + (float)(selection.selection_end.y + 1) * tile_size};
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

                    static bool pressed_last_frame = false;
                    if (io.MouseDown[ImGuiMouseButton_Left]) {
                        if (!pressed_last_frame) {
                            selection.selection_start = {(int)(relative_mouse_pos.x / tile_size),
                                                         (int)(relative_mouse_pos.y / tile_size)};
                            selection.selection_end = selection.selection_start;
                        } else {
                            selection.selection_end = {
                                std::max(selection.selection_start.x,
                                         (int)(relative_mouse_pos.x / tile_size)),
                                std::max(selection.selection_start.y,
                                         (int)(relative_mouse_pos.y / tile_size))};
                        }
                    }
                    pressed_last_frame = io.MouseDown[ImGuiMouseButton_Left];
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
                    ImGui::Image(reinterpret_cast<ImTextureID>(img->handle), img_size, uv_min,
                                 uv_max, ImVec4(1, 1, 1, tooltip_alpha));
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

            } else
                ImGui::TextDisabled("No texture attached to tileset.");
        } else
            ImGui::TextDisabled("No tileset loaded.");
    }
    ImGui::End();

    static bool show_new_tileset = false;
    if (ImGui::Begin("Tileset List", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...")) {
                show_new_tileset = true;
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
                    if (ImGui::Selectable(tileset->name.c_str(), _t == selection.tileset)) {
                        selection.tileset = _t;
                        update_grid_texture();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, 0.f, 0.f, 1.f});
                    ImGui::TextUnformatted("Empty reference");
                    ImGui::PopStyleColor(1);
                }
            }
    }
    ImGui::End();

    if (show_new_tileset) {
        if (ImGui::Begin("New Tileset")) {
            static char path_selected[4096] = "\0";
            ImGui::InputTextWithHint("Path", "Enter path...", path_selected, 4096);
            ImGui::SameLine();
            if (ImGui::Button("Explore...")) {
                const char* compatible_file_formats =
                    "Any file\0*\0JPEG\0*jpg;*.jpeg\0PNG\0*png\0TGA\0*.tga\0BMP\0*.bmp\0PSD\0"
                    "*.psd\0GIF\0*.gif\0HDR\0*.hdr\0PIC\0*.pic\0PNM\0*.pnm\0";
                const char* noc_path_selected = noc_file_dialog_open(
                    NOC_FILE_DIALOG_OPEN, compatible_file_formats, nullptr, nullptr);
                if (noc_path_selected && fs::is_regular_file(noc_path_selected)) {
                    strcpy(path_selected, noc_path_selected);
                }
            }

            static auto auto_type = assets::Tileset::AutoType::none;
            static const char* auto_type_bindings[] = {"None", "RPGMaker Auto Tileset (A1/A2)"};
            constexpr u32 auto_type_bindings_count = 2;
            static_assert(auto_type_bindings_count == (u32)assets::Tileset::AutoType::count);
            if (ImGui::BeginCombo("Type", auto_type_bindings[static_cast<u32>(auto_type)])) {
                for (u32 i = 0; i < auto_type_bindings_count; i++) {
                    if (ImGui::Selectable(auto_type_bindings[i]))
                        auto_type = static_cast<assets::Tileset::AutoType>(i);
                    if (static_cast<assets::Tileset::AutoType>(i) == auto_type)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const bool valid = fs::is_regular_file(path_selected);
            if (!valid) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button("OK")) {
                assets::Tileset tileset;
                tileset.name = fs::path(path_selected).filename();
                tileset.auto_type = auto_type;
                switch (auto_type) {
                    case (assets::Tileset::AutoType::none):
                        tileset.texture =
                            asset_manager::load<assets::Texture>({path_selected, false});
                        break;

                    case (assets::Tileset::AutoType::rpgmaker_a12): {
                        auto raw_texture =
                            asset_manager::load<assets::Texture>({path_selected, false});
                        tileset.texture =
                            calculate_rpgmaker_a12_auto_tileset_texture(*raw_texture.get());
                        raw_texture.unload();
                        break;
                    }

                    default:
                        assert(false); // not implemented, outta here
                        break;
                }
                selection.tileset = tilesets.emplace_back(asset_manager::put(tileset));
                update_grid_texture();
                show_new_tileset = false;
            }
            if (!valid) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
        }
        ImGui::End();
    }
}

u32 get_tile_size() { return tile_size; }

std::vector<asset_manager::Handle<assets::Tileset>>& get_tilesets() { return tilesets; }

TilesetSelection& get_selection() { return selection; }

} // namespace arpiyi_editor::tileset_manager