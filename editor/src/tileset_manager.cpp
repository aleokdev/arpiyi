#include "tileset_manager.hpp"
#include "window_list_menu.hpp"
#include "window_manager.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <noc_file_dialog.h>
#include <vector>

#include "assets/map.hpp"
#include "util/defs.hpp"
#include "util/icons_material_design.hpp"
#include "util/math.hpp"

#include "assets/shader.hpp"

namespace arpiyi::tileset_manager {

Handle<assets::Shader> tile_shader;
Handle<assets::Shader> grid_shader;
Handle<assets::Shader> uv_tile_shader;
Handle<assets::Mesh> quad_mesh;
std::size_t tile_size = 48;

unsigned int grid_framebuffer;
assets::Texture grid_texture;

glm::mat4 proj_mat;

TilesetSelection selection{-1, {0, 0}, {0, 0}};

static void update_grid_texture() {
    auto tileset = selection.tileset.get();
    if (!tileset)
        return;

    if (grid_texture.handle == assets::Texture::nohandle)
        glDeleteTextures(1, &grid_texture.handle);
    glGenTextures(1, &grid_texture.handle);
    glBindTexture(GL_TEXTURE_2D, grid_texture.handle);

    const auto tileset_size = tileset->get_size_in_tiles();
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
    ImVec4 bg_color = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_WindowBg));
    glUniform4f(3, bg_color.x, bg_color.y, bg_color.z, 1.f); // Grid color
    glUniform2ui(4, tileset_size.x, tileset_size.y);
    glViewport(0.f, 0.f, grid_texture.w, grid_texture.h);
    glm::mat4 model = glm::mat4(1);
    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, quad_verts);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Handle<assets::Texture>
calculate_rpgmaker_a2_auto_tileset_texture(assets::Texture const& source_tex) {
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
    const math::IVec2D rpgmaker_tileset_size_in_tilesets{rpgmaker_tileset_size_in_tiles.x / 2,
                                                         rpgmaker_tileset_size_in_tiles.y / 3};
    const i32 rpgmaker_tileset_number_of_tilesets =
        rpgmaker_tileset_size_in_tilesets.x * rpgmaker_tileset_size_in_tilesets.y;
    glm::mat4 tex_mat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

    // 64 tiles are needed to cache all possible combinations of corners.
    // HOWEVER, some of them are equal. RPGMaker-style texture only uses corner textures when there
    // aren't sides next to them. By doing some math, we end with the conclusion that only *47*
    // tiles are needed. Since 47 can't be nicely split into a square/rectangle texture, we'll just
    // do a 1*tileset_n*tile_size x 47*tile_size texture.
    unsigned int temp_fb;
    glGenFramebuffers(1, &temp_fb);
    assets::Texture generated_texture;
    glGenTextures(1, &generated_texture.handle);
    glBindTexture(GL_TEXTURE_2D, generated_texture.handle);
    generated_texture.w = tile_size * rpgmaker_tileset_number_of_tilesets;
    generated_texture.h = 47 * tile_size;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, generated_texture.w, generated_texture.h, 0, GL_RGBA,
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

    enum tile_sides {
        upper_left_corner = 1 << 0,
        upper_middle_side = 1 << 1,
        upper_right_corner = 1 << 2,
        middle_left_side = 1 << 3,
        middle_right_side = 1 << 4,
        lower_left_corner = 1 << 5,
        lower_middle_side = 1 << 6,
        lower_right_corner = 1 << 7
    };

    for (i32 tileset_i = 0;
         tileset_i < rpgmaker_tileset_size_in_tilesets.x * rpgmaker_tileset_size_in_tilesets.y;
         tileset_i++) {
        u8 tile_index = 0;
        // Calculate each corner variation texture and draw it to the framebuffer.
        for (u8 corner_variation = 0; corner_variation < 0xFF; corner_variation++) {
            // For each bit of corner_variation, 1 is "tile of same type adjacent in that
            // corner/side", 0 is the opposite.

            if ((corner_variation & upper_left_corner) &&
                ((corner_variation & upper_middle_side) || (corner_variation & middle_left_side)))
                continue;
            if ((corner_variation & upper_right_corner) &&
                ((corner_variation & upper_middle_side) || (corner_variation & middle_right_side)))
                continue;
            if ((corner_variation & lower_left_corner) &&
                ((corner_variation & lower_middle_side) || (corner_variation & middle_left_side)))
                continue;
            if ((corner_variation & lower_right_corner) &&
                ((corner_variation & lower_middle_side) || (corner_variation & middle_right_side)))
                continue;

            constexpr u8 minitile_count = 4;
            // Let's first go through each one of the minitiles in the corner variation.
            for (u8 minitile = 0; minitile < minitile_count; minitile++) {
                // "case" is used here to identify the type of minitile. Check out:
                // https://dat5n5oxdq-flywheel.netdna-ssl.com/wp-content/uploads/2012/01/AT-Organization.png
                // Or alternatively, the image at
                // https://blog.rpgmakerweb.com/tutorials/anatomy-of-an-autotile/ Which binds each
                // minitile to a number and letter.
                // A minitile can be represented with a letter and a number. The letters go from A
                // to D and the numbers go from 1 to 5. The letters represent the location of the
                // minitile relative to the parent tile; The numbers represent the corner variation
                // of that minitile.
                // "A" minitiles represent those placed in the upper left part of the tile.
                // "B" minitiles represent those placed in the upper right part of the tile.
                // "C" minitiles represent those placed in the lower left part of the tile.
                // "D" minitiles represent those placed in the lower right part of the tile.
                // "1" minitiles represent those with two adjacent tiles (On both sides of the
                // minitile)
                // "2" minitiles represent those with no adjacent tiles (On both sides of the
                // minitile)
                // "3" minitiles represent those with every adjacent tile (On both sides of the
                // minitile & on the corner)
                // "4" minitiles represent those with an adjacent tile in their left or right,
                // depending on the minitile.
                // "5" minitiles represent those with an adjacent tile above or below, depending on
                // the minitile.

                // Represents the adjacent tiles for this minitile.
                bool has_corner{};
                bool has_vert_side{};
                bool has_horz_side{};

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
                    default: ARPIYI_UNREACHABLE();
                }

                const float source_tile_uv_size_x =
                                1.f / static_cast<float>(rpgmaker_tileset_size_in_tiles.x),
                            source_tile_uv_size_y =
                                1.f / static_cast<float>(rpgmaker_tileset_size_in_tiles.y);

                float source_minitile_relative_x, source_minitile_relative_y;
                if (!has_vert_side && !has_horz_side && has_corner) { // "1" case
                    source_minitile_relative_x =
                        source_tile_uv_size_x / 2.f * static_cast<float>((minitile % 2) + 2);
                    source_minitile_relative_y =
                        source_tile_uv_size_y / 2.f * static_cast<float>(minitile / 2);
                } else { // Any other case ("2" to "5")
                    enum MinitileFacing { right = 0b01, down = 0b10 };

                    float minitile_pos_x = static_cast<float>(
                              (minitile % 2) +
                              (has_horz_side ? ((minitile & MinitileFacing::right) ? 2 : 0)
                                             : ((minitile & MinitileFacing::right) ? 0 : 2))),
                          minitile_pos_y = static_cast<float>(
                              (minitile / 2) + (has_vert_side
                                                    ? ((minitile & MinitileFacing::down) ? 2 : 0)
                                                    : ((minitile & MinitileFacing::down) ? 0 : 2)));

                    source_minitile_relative_x = source_tile_uv_size_x / 2.f * minitile_pos_x;
                    source_minitile_relative_y = source_tile_uv_size_y / 2.f * (minitile_pos_y + 2);
                }

                source_minitile_relative_x += 1.f / rpgmaker_tileset_size_in_tilesets.x *
                                              (tileset_i % rpgmaker_tileset_size_in_tilesets.x);
                source_minitile_relative_y += 1.f / rpgmaker_tileset_size_in_tilesets.y *
                                              (tileset_i / rpgmaker_tileset_size_in_tilesets.x);

                const float target_tile_uv_x_scale =
                    1.f / static_cast<float>(rpgmaker_tileset_number_of_tilesets);
                constexpr float target_tile_uv_y_scale = 1.f / 47.f;
                const float target_minitile_x =
                                target_tile_uv_x_scale * (static_cast<float>(tileset_i) +
                                                          static_cast<float>(minitile % 2) / 2.f),
                            target_minitile_y =
                                target_tile_uv_y_scale * (static_cast<float>(tile_index) +
                                                          static_cast<float>(minitile / 2) / 2.f);
                glm::mat4 model = glm::mat4(1);
                model = glm::translate(model, glm::vec3(target_minitile_x, target_minitile_y, 0));
                model = glm::scale(model, glm::vec3(target_tile_uv_x_scale / 2.f,
                                                    target_tile_uv_y_scale / 2.f, 1));
                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(tex_mat));
                glUniform2f(3, source_minitile_relative_x, source_minitile_relative_y); // UV start
                glUniform2f(4, source_minitile_relative_x + source_tile_uv_size_x / 2.f,
                            source_minitile_relative_y + source_tile_uv_size_y / 2.f); // UV end
                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0, quad_verts);
            }
            tile_index++;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &temp_fb);

    return asset_manager::put(generated_texture);
}

void init() {
    glGenFramebuffers(1, &grid_framebuffer);

    tile_shader = asset_manager::load<assets::Shader>({"data/basic.vert", "data/basic.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    uv_tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile_uv.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, 0.0f);
    window_list_menu::add_entry({"Tileset view", &render});
}

void render(bool* p_show) {
    if (ImGui::Begin(ICON_MD_BORDER_INNER " Tileset View", nullptr,
                     ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {
        if (auto ts = selection.tileset.get()) {
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
                static float tooltip_alpha = 0;
                bool update_tooltip_info;
                if (ImGui::IsWindowHovered(ImGuiFocusedFlags_RootWindow) &&
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
                if (tooltip_alpha > 0.01f) {
                    ImGui::BeginTooltip();
                    {
                        const math::IVec2D tile_hovering{(int)(relative_mouse_pos.x / tile_size),
                                                         (int)(relative_mouse_pos.y / tile_size)};
                        const ImVec2 img_size{64, 64};
                        const math::IVec2D size_in_tiles = ts->get_size_in_tiles();
                        const ImVec2 uv_min{(float)tile_hovering.x / (float)size_in_tiles.x,
                                            (float)tile_hovering.y / (float)size_in_tiles.y};
                        const ImVec2 uv_max{(float)(tile_hovering.x + 1) / (float)size_in_tiles.x,
                                            (float)(tile_hovering.y + 1) / (float)size_in_tiles.y};
                        ImGui::Image(reinterpret_cast<ImTextureID>(img->handle), img_size, uv_min,
                                     uv_max, ImVec4(1, 1, 1, tooltip_alpha));
                        ImGui::SameLine();
                        static std::size_t tile_id;
                        if (update_tooltip_info)
                            tile_id = tile_hovering.x +
                                      tile_hovering.y * ts->get_size_in_tiles().x;
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, .8f, .8f, tooltip_alpha});
                        ImGui::Text("ID %zu", tile_id);
                        ImGui::Text("UV coords: {%.2f~%.2f, %.2f~%.2f}", uv_min.x, uv_max.x,
                                    uv_min.y, uv_max.y);
                        ImGui::PopStyleColor(1);
                    }
                    ImGui::EndTooltip();
                }
            } else
                ImGui::TextDisabled("No texture attached to tileset.");
        } else
            ImGui::TextDisabled("No tileset loaded.");
    }
    ImGui::End();

    static bool show_new_tileset = false;
    if (ImGui::Begin(ICON_MD_LIBRARY_BOOKS " Tileset List", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...")) {
                show_new_tileset = true;
            }
            ImGui::EndMenuBar();
        }
        if (detail::AssetContainer<assets::Tileset>::get_instance().map.empty())
            ImGui::TextDisabled("No tilesets");
        else {
            int id_to_delete = -1;
            for (auto& [_id, _t] : detail::AssetContainer<assets::Tileset>::get_instance().map) {
                ImGui::TextDisabled("%zu", _id);
                ImGui::SameLine();
                std::string selectable_strid = _t.name;
                selectable_strid += "##";
                selectable_strid += std::to_string(_id);
                if (ImGui::Selectable(selectable_strid.c_str(),
                                      _id == selection.tileset.get_id())) {
                    selection.tileset = Handle<assets::Tileset>(_id);
                    update_grid_texture();
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::Selectable("Delete")) {
                        id_to_delete = _id;
                        selection.tileset = Handle<assets::Tileset>(Handle<assets::Tileset>::noid);
                    }
                    ImGui::EndPopup();
                }
            }

            if (id_to_delete != -1) {
                Handle<assets::Tileset>(id_to_delete).unload();
                detail::AssetContainer<assets::Tileset>::get_instance().map.erase(id_to_delete);
            }
        }
    }
    ImGui::End();

    if (show_new_tileset) {
        if (ImGui::Begin(ICON_MD_LIBRARY_ADD " New Tileset", &show_new_tileset)) {
            static Handle<assets::Texture> preview_texture;
            static char path_selected[4096] = "\0";
            if (ImGui::InputTextWithHint("Path", "Enter path...", path_selected, 4096,
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                preview_texture.unload();
                if (fs::is_regular_file(path_selected))
                    preview_texture = asset_manager::load<assets::Texture>({path_selected});
            }
            ImGui::SameLine();
            if (ImGui::Button("Explore...")) {
                const char* compatible_file_formats =
                    "Any file\0*\0JPEG\0*jpg;*.jpeg\0PNG\0*png\0TGA\0*.tga\0BMP\0*.bmp\0PSD\0"
                    "*.psd\0GIF\0*.gif\0HDR\0*.hdr\0PIC\0*.pic\0PNM\0*.pnm\0";
                const char* noc_path_selected = noc_file_dialog_open(
                    NOC_FILE_DIALOG_OPEN, compatible_file_formats, nullptr, nullptr);
                if (noc_path_selected && fs::is_regular_file(noc_path_selected)) {
                    strcpy(path_selected, noc_path_selected);
                    preview_texture.unload();
                    preview_texture = asset_manager::load<assets::Texture>({path_selected});
                }
            }
            bool valid = preview_texture.get();

            static auto auto_type = assets::Tileset::AutoType::none;
            static const char* auto_type_bindings[] = {"Normal", "RPGMaker A2 Tileset"};
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

            static int input_tile_size = tile_size;
            bool can_modify_tile_size =
                detail::AssetContainer<assets::Tileset>::get_instance().map.empty();
            if (can_modify_tile_size) {
                ImGui::InputInt("Tile size", &input_tile_size);
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(240.f);
                    ImGui::TextUnformatted(
                        "The size in pixels of each tile in the tileset.\nIMPORTANT: This option "
                        "is only available when there are no other tilesets loaded in, so think "
                        "the size carefully before setting it. For reference, RPGMaker VX/VX Ace "
                        "uses 32x32 tiles, RPGMaker MV uses 48x48 tiles, and most pixel art uses "
                        "16x16 or 32x32.");
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                valid &= tile_size > 0 && tile_size < 1024;
            }

            auto tex = preview_texture.get();
            if (tex) {
                if (tex->w % input_tile_size != 0 || tex->h % input_tile_size != 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, {1, .1f, .1f, 1});
                    ImGui::TextWrapped(
                        ICON_MD_ERROR
                        " This doesn't look like a tileset of %ix%i-sized tiles. Double check your "
                        "tileset and tile size.\nTilesets must have width and height that are "
                        "dividable by the tile size leaving no remainder.",
                        input_tile_size, input_tile_size);
                    ImGui::PopStyleColor();
                    valid = false;
                }
                if (auto_type == assets::Tileset::AutoType::rpgmaker_a2) {
                    if (tex->w % (2 * input_tile_size) != 0 ||
                        tex->h % (3 * input_tile_size) != 0) {
                        ImGui::PushStyleColor(ImGuiCol_Text, {1, .1f, .1f, 1});
                        ImGui::TextWrapped(
                            ICON_MD_ERROR
                            " This image doesn't look like a proper RPGMaker A2 tileset.\nRPGMaker "
                            "A2 tilesets must have a width that is multiple of 2 * tilesize and a "
                            "height that is multiple of 3 * tilesize.");
                        ImGui::PopStyleColor();
                        valid = false;
                    }
                }
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, {1, .1f, .1f, 1});
                ImGui::TextWrapped(ICON_MD_ERROR
                                   " Please input a valid path for the tileset image.");
                ImGui::PopStyleColor();
                valid = false;
            }

            if (ImGui::Button("Cancel"))
                show_new_tileset = false;
            ImGui::SameLine();
            if (!valid) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button("OK")) {
                if (can_modify_tile_size)
                    tile_size = static_cast<std::size_t>(input_tile_size);

                assets::Tileset tileset;
                tileset.name = fs::path(path_selected).filename().generic_string();
                tileset.auto_type = auto_type;
                switch (auto_type) {
                    case (assets::Tileset::AutoType::none):
                        tileset.texture = preview_texture;
                        break;

                    case (assets::Tileset::AutoType::rpgmaker_a2): {
                        tileset.texture =
                            calculate_rpgmaker_a2_auto_tileset_texture(*preview_texture.get());
                        preview_texture.unload();
                        break;
                    }

                    default:
                        assert(false); // not implemented, outta here
                        break;
                }
                selection.tileset = asset_manager::put(tileset);
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

void set_tile_size(u32 size) { tile_size = size; }
u32 get_tile_size() { return tile_size; }

std::vector<Handle<assets::Tileset>> get_tilesets() {
    std::vector<Handle<assets::Tileset>> tilesets;
    for (const auto& [id, tileset] : detail::AssetContainer<assets::Tileset>::get_instance().map) {
        tilesets.emplace_back(id);
    }
    return tilesets;
}

TilesetSelection const& get_selection() { return selection; }
void set_selection_tileset(Handle<assets::Tileset> tileset) {
    selection.tileset = tileset;
    selection.selection_start = {0, 0};
    selection.selection_end = {0, 0};

    update_grid_texture();
}

} // namespace arpiyi::tileset_manager