#include "sprite_manager.hpp"
#include "global_tile_size.hpp"
#include "tileset_manager.hpp"
#include "window_list_menu.hpp"

#include "assets/sprite.hpp"

#include "util/icons_material_design.hpp"
#include "util/math.hpp"

#include <cmath>

#include <imgui.h>
#include <imgui_internal.h>

namespace arpiyi::sprite_manager {

void init() { window_list_menu::add_entry({"Sprite List", &render}); }

void render(bool* p_show) {
    static bool show_add_sprite_from_tileset_window = false;
    if (ImGui::Begin(ICON_MD_WALLPAPER " Sprite List", p_show, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Add")) {
                if (ImGui::MenuItem("From tileset...")) {
                    show_add_sprite_from_tileset_window = true;
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        for (const auto& _t : tileset_manager::get_tilesets()) {
            assert(_t.get());
            const auto& tileset = *_t.get();
            std::string tree_node_name;
            tree_node_name.reserve(32);
            tree_node_name += ICON_MD_BORDER_INNER " ";
            tree_node_name += tileset.name;
            if (ImGui::TreeNode(tree_node_name.c_str())) {
                math::IVec2D size_in_tiles = tileset.size_in_tile_units();
                for (int y = 0; y < size_in_tiles.y; ++y)
                    for (int x = 0; x < size_in_tiles.x; ++x) {
                        char selectable_text[32];
                        sprintf(selectable_text, "Tile {%i, %i}", x, y);
                        ImGui::PushStyleColor(ImGuiCol_Text,
                                              ImGui::GetColorU32(ImGuiCol_TextDisabled));
                        ImGui::Selectable(selectable_text);
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::PushTextWrapPos(225.f);
                            ImGui::TextUnformatted(
                                "This is a tile preview, not a sprite preview. If you'd like to "
                                "add this image as a sprite, choose Add > From tileset.");
                            ImGui::PopTextWrapPos();
                            const ImVec2 uv_min{(float)x / (float)size_in_tiles.x,
                                                (float)y / (float)size_in_tiles.y};
                            const ImVec2 uv_max{(float)(x + 1) / (float)size_in_tiles.x,
                                                (float)(y + 1) / (float)size_in_tiles.y};
                            ImGui::Image(
                                reinterpret_cast<ImTextureID>(tileset.texture.get()->handle),
                                ImVec2{static_cast<float>(global_tile_size::get()),
                                       static_cast<float>(global_tile_size::get())},
                                uv_min, uv_max);
                            ImGui::EndTooltip();
                        }
                    }
                ImGui::TreePop();
            }
        }

        // TODO: Reimplement
        /*
        for (const auto& [_id, sprite] :
             detail::AssetContainer<assets::Sprite>::get_instance().map) {
            ImGui::TextDisabled("%zu", _id);
            ImGui::SameLine();
            ImGui::Selectable(sprite.name.c_str());
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                math::IVec2D sprite_px_size = sprite.get_size_in_pixels();
                ImGui::Image(reinterpret_cast<ImTextureID>(sprite.texture.get()->handle),
                             ImVec2{static_cast<float>(sprite_px_size.x),
                                    static_cast<float>(sprite_px_size.y)},
                             {sprite.uv_min.x, sprite.uv_min.y},
                             {sprite.uv_max.x, sprite.uv_max.y});
                ImGui::EndTooltip();
            }
        }*/
    }
    ImGui::End();

    if (show_add_sprite_from_tileset_window) {
        if (ImGui::Begin(ICON_MD_ADD_BOX " Add Sprite From Tileset", nullptr,
                         ImGuiWindowFlags_NoScrollbar)) {
            static Handle<assets::Tileset> tileset = -1;
            auto t = tileset.get();
            if (ImGui::BeginCombo("Tileset", t ? t->name.c_str() : "<Not selected>")) {
                for (auto& ts : tileset_manager::get_tilesets()) {
                    if (ImGui::Selectable(ts.get()->name.c_str(), ts == tileset))
                        tileset = ts;

                    if (ts == tileset)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            static char name_buf[128] = "Sprite";
            ImGui::InputText("Sprite Name", name_buf, 128);

            static math::IVec2D tileset_tile_selection_start = {0, 0};
            static math::IVec2D tileset_tile_selection_end = {0, 0};
            if (t) {
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
                ImGui::BeginChild("Tileset Preview##spr_t",
                                  {0, -ImGui::GetTextLineHeightWithSpacing()}, true,
                                  ImGuiWindowFlags_HorizontalScrollbar);
                assert(t->texture.get());
                auto tex = *t->texture.get();
                ImVec2 tileset_image_cursor_pos = ImGui::GetCursorScreenPos();
                // Draw tileset image
                ImGui::Image(reinterpret_cast<ImTextureID>(tex.handle),
                             ImVec2{static_cast<float>(tex.w), static_cast<float>(tex.h)});

                if (ImGui::IsItemClicked()) {
                    const auto mouse_pos = ImGui::GetMousePos();
                    tileset_tile_selection_start.x = tileset_tile_selection_end.x =
                        ((mouse_pos.x - tileset_image_cursor_pos.x) -
                         static_cast<float>(
                             static_cast<i32>(mouse_pos.x - tileset_image_cursor_pos.x) %
                             global_tile_size::get())) /
                        global_tile_size::get();
                    tileset_tile_selection_start.y = tileset_tile_selection_end.y =
                        ((mouse_pos.y - tileset_image_cursor_pos.y) -
                         static_cast<float>(
                             static_cast<i32>(mouse_pos.y - tileset_image_cursor_pos.y) %
                             global_tile_size::get())) /
                        global_tile_size::get();
                }
                if (ImGui::IsItemHovered() && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                    const auto mouse_pos = ImGui::GetMousePos();
                    tileset_tile_selection_end.x = std::max(
                        tileset_tile_selection_start.x,
                        static_cast<i32>(
                            ((mouse_pos.x - tileset_image_cursor_pos.x) -
                             static_cast<float>(
                                 static_cast<i32>(mouse_pos.x - tileset_image_cursor_pos.x) %
                                 global_tile_size::get())) /
                            global_tile_size::get()));
                    tileset_tile_selection_end.y = std::max(
                        tileset_tile_selection_start.y,
                        static_cast<i32>(
                            ((mouse_pos.y - tileset_image_cursor_pos.y) -
                             static_cast<float>(
                                 static_cast<i32>(mouse_pos.y - tileset_image_cursor_pos.y) %
                                 global_tile_size::get())) /
                            global_tile_size::get()));
                }

                // Draw selection box
                ImVec2 selection_pos_min{
                    tileset_image_cursor_pos.x + static_cast<float>(tileset_tile_selection_start.x *
                                                                    global_tile_size::get()),
                    tileset_image_cursor_pos.y + static_cast<float>(tileset_tile_selection_start.y *
                                                                    global_tile_size::get())};
                ImVec2 selection_pos_max{tileset_image_cursor_pos.x +
                                             static_cast<float>((tileset_tile_selection_end.x + 1) *
                                                                global_tile_size::get()),
                                         tileset_image_cursor_pos.y +
                                             static_cast<float>((tileset_tile_selection_end.y + 1) *
                                                                global_tile_size::get())};
                auto draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRect(selection_pos_min, selection_pos_max, 0xFFFFFFFF, 0,
                                   ImDrawCornerFlags_All, 4.f);
                draw_list->AddRect(selection_pos_min, selection_pos_max, 0xFF000000, 0,
                                   ImDrawCornerFlags_All, 2.f);
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            if (ImGui::Button("Cancel")) {
                show_add_sprite_from_tileset_window = false;
            }

            ImGui::SameLine();
            if (!t) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button("OK")) {
                // TODO: Reimplement
                /*
                aml::Vector2 tileset_uv_min{static_cast<float>(tileset_tile_selection_start.x) /
                                             static_cast<float>(t->get_size_in_tiles().x),
                                         static_cast<float>(tileset_tile_selection_start.y) /
                                             static_cast<float>(t->get_size_in_tiles().y)};
                aml::Vector2 tileset_uv_max{static_cast<float>(tileset_tile_selection_end.x + 1) /
                                             static_cast<float>(t->get_size_in_tiles().x),
                                         static_cast<float>(tileset_tile_selection_end.y + 1) /
                                             static_cast<float>(t->get_size_in_tiles().y)};
                asset_manager::put(
                    assets::Sprite{t->texture, tileset_uv_min, tileset_uv_max, name_buf});
                show_add_sprite_from_tileset_window = false;
                 */
            }
            if (!t) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
        }
        ImGui::End();
    }
}

} // namespace arpiyi::sprite_manager