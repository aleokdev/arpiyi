#include "tileset_manager.hpp"
#include "window_list_menu.hpp"
#include "window_manager.hpp"

#include <algorithm>
#include <imgui.h>
#include <imgui_internal.h>
#include <noc_file_dialog.h>
#include <vector>

#include "assets/map.hpp"
#include "global_tile_size.hpp"
#include "util/defs.hpp"
#include "util/icons_material_design.hpp"
#include "util/math.hpp"

#include <anton/math/matrix4.hpp>
#include <anton/math/transform.hpp>

namespace aml = anton::math;

namespace arpiyi::tileset_manager {

constexpr const char* tileset_view_strid = ICON_MD_BORDER_INNER " Tileset View";
static renderer::Framebuffer tileset_fb;

TilesetSelection selection{-1, {0, 0}, {0, 0}};

static void render_tileset() {
    window_manager::get_renderer().clear(tileset_fb, {0, 0, 0, 0});
    renderer::MeshBuilder builder;
    if (auto t = selection.tileset.get()) {
        const auto& tileset = *t;
        const auto tileset_size = tileset.size_in_tile_units();
        // TODO: CACHE!!!!!!
        for (u64 tile_i = 0; tile_i < tileset.tile_count(); ++tile_i) {
            builder.add_sprite(assets::Tileset::Tile{selection.tileset, tile_i}.preview_sprite(),
                               {static_cast<float>(tile_i % tileset_size.x),
                                -static_cast<float>(tile_i / tileset_size.x) - 1.f, 0},
                               0, 0);
        }
        auto mesh = builder.finish();
        renderer::DrawCmdList commands{
            {{static_cast<float>(ImGui::GetScrollX()) / global_tile_size::get(),
              -static_cast<float>(ImGui::GetScrollY()) / global_tile_size::get(), 0},
             1,
             false},
            {renderer::DrawCmd{tileset.texture.get()->handle,
                               mesh,
                               window_manager::get_renderer().unlit_shader(),
                               {{0, 0, 0}}}}};
        window_manager::get_renderer().draw(commands, tileset_fb);
        mesh.unload();
    }
}

void init() { window_list_menu::add_entry({"Tileset view", &render}); }

void render(bool* p_show) {
    if (ImGui::Begin(tileset_view_strid, nullptr,
                     ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar)) {
        if (auto ts = selection.tileset.get()) {
            if (auto img = ts->texture.get()) {
                const ImVec2 tileset_render_pos = {ImGui::GetCursorScreenPos().x,
                                                   ImGui::GetCursorScreenPos().y};
                const math::IVec2D tileset_size_tile_units = ts->size_in_tile_units();
                const math::IVec2D tileset_size_actual_tile_size = {
                    tileset_size_tile_units.x,
                    static_cast<i32>(ts->tile_count() / tileset_size_tile_units.x)};
                const ImVec2 tileset_render_pos_max =
                    ImVec2(tileset_render_pos.x +
                               tileset_size_actual_tile_size.x * global_tile_size::get(),
                           tileset_render_pos.y +
                               tileset_size_actual_tile_size.y * global_tile_size::get());

                const auto& io = ImGui::GetIO();
                ImVec2 mouse_pos = io.MousePos;
                ImVec2 relative_mouse_pos =
                    ImVec2(mouse_pos.x - tileset_render_pos.x, mouse_pos.y - tileset_render_pos.y);
                // Snap the relative mouse position
                relative_mouse_pos.x = (int)relative_mouse_pos.x -
                                       ((int)relative_mouse_pos.x % global_tile_size::get());
                relative_mouse_pos.y = (int)relative_mouse_pos.y -
                                       ((int)relative_mouse_pos.y % global_tile_size::get());
                // Apply the same snapping effect to the regular mouse pos
                mouse_pos = ImVec2(relative_mouse_pos.x + tileset_render_pos.x,
                                   relative_mouse_pos.y + tileset_render_pos.y);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                static ImVec2 last_window_size;
                // Update the framebuffer if the window has changed size
                if (ImGui::GetWindowSize().x != last_window_size.x ||
                    ImGui::GetWindowSize().y != last_window_size.y) {
                    if (!tileset_fb.exists()) {
                        using Tex = renderer::TextureHandle;
                        Tex tex;
                        tex.init(static_cast<int>(ImGui::GetWindowContentRegionWidth()),
                                 static_cast<int>(ImGui::GetWindowContentRegionMax().y -
                                                  ImGui::GetWindowContentRegionMin().y),
                                 Tex::ColorType::rgba, Tex::FilteringMethod::point);
                        tileset_fb = renderer::Framebuffer(tex);
                    } else {
                        tileset_fb.resize({static_cast<int>(ImGui::GetWindowContentRegionWidth()),
                                           static_cast<int>(ImGui::GetWindowContentRegionMax().y -
                                                            ImGui::GetWindowContentRegionMin().y)});
                    }

                    last_window_size = ImGui::GetWindowSize();
                }
                render_tileset();
                ImGui::Dummy(ImVec2{(float)img->handle.width(), (float)img->handle.height()});
                ImGui::SetCursorScreenPos({tileset_render_pos.x + ImGui::GetScrollX(),
                                           tileset_render_pos.y + ImGui::GetScrollY()});
                ImGui::Image(tileset_fb.texture().imgui_id(),
                             ImVec2{(float)tileset_fb.texture().width(),
                                    (float)tileset_fb.texture().height()},
                             {0, 1}, {1, 0});

                // Clip anything that is outside the tileset rect
                draw_list->PushClipRect(tileset_render_pos, tileset_render_pos_max, true);

                const ImVec2 tile_selection_start_rel =
                    ImVec2{tileset_render_pos.x +
                               (float)selection.selection_start.x * global_tile_size::get(),
                           tileset_render_pos.y +
                               (float)selection.selection_start.y * global_tile_size::get()};
                const ImVec2 tile_selection_end_rel =
                    ImVec2{tileset_render_pos.x +
                               (float)(selection.selection_end.x + 1) * global_tile_size::get(),
                           tileset_render_pos.y +
                               (float)(selection.selection_end.y + 1) * global_tile_size::get()};
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
                                       {mouse_pos.x + global_tile_size::get(),
                                        mouse_pos.y + global_tile_size::get()},
                                       0xFFFFFFFF, 0, ImDrawCornerFlags_All, 4.f);
                    draw_list->AddRect(mouse_pos,
                                       {mouse_pos.x + global_tile_size::get(),
                                        mouse_pos.y + global_tile_size::get()},
                                       0xFF000000, 0, ImDrawCornerFlags_All, 2.f);

                    draw_list->PopClipRect();

                    static bool pressed_last_frame = false;
                    if (io.MouseDown[ImGuiMouseButton_Left]) {
                        if (!pressed_last_frame) {
                            selection.selection_start = {
                                (int)(relative_mouse_pos.x / global_tile_size::get()),
                                (int)(relative_mouse_pos.y / global_tile_size::get())};
                            selection.selection_end = selection.selection_start;
                        } else {
                            selection.selection_end = {
                                std::max(selection.selection_start.x,
                                         (int)(relative_mouse_pos.x / global_tile_size::get())),
                                std::max(selection.selection_start.y,
                                         (int)(relative_mouse_pos.y / global_tile_size::get()))};
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
                        const math::IVec2D tile_hovering{
                            (int)(relative_mouse_pos.x / global_tile_size::get()),
                            (int)(relative_mouse_pos.y / global_tile_size::get())};
                        const ImVec2 img_size{64, 64};
                        const math::IVec2D size_in_tiles = ts->size_in_tile_units();
                        const ImVec2 uv_min{(float)tile_hovering.x / (float)size_in_tiles.x,
                                            (float)tile_hovering.y / (float)size_in_tiles.y};
                        const ImVec2 uv_max{(float)(tile_hovering.x + 1) / (float)size_in_tiles.x,
                                            (float)(tile_hovering.y + 1) / (float)size_in_tiles.y};
                        ImGui::Image(img->handle.imgui_id(), img_size, uv_min, uv_max,
                                     ImVec4(1, 1, 1, tooltip_alpha));
                        ImGui::SameLine();
                        static std::size_t tile_id;
                        if (update_tooltip_info)
                            tile_id =
                                tile_hovering.x + tile_hovering.y * ts->size_in_tile_units().x;
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
                    ImGui::SetWindowFocus(tileset_view_strid);
                    selection.tileset = Handle<assets::Tileset>(_id);
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
            static Handle<assets::TextureAsset> preview_texture;
            static char path_selected[4096] = "\0";
            if (ImGui::InputTextWithHint("Path", "Enter path...", path_selected, 4096,
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                preview_texture.unload();
                if (fs::is_regular_file(path_selected))
                    preview_texture = asset_manager::load<assets::TextureAsset>({path_selected});
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
                    preview_texture = asset_manager::load<assets::TextureAsset>({path_selected});
                }
            }
            bool valid = preview_texture.get();

            static auto auto_type = assets::TileType::normal;
            static const char* auto_type_bindings[] = {"Normal", "RPGMaker A2 Tileset", "RPGMaker A4 Tileset"};
            constexpr u32 auto_type_bindings_count = 3;
            static_assert(auto_type_bindings_count == (u32)assets::TileType::count);
            if (ImGui::BeginCombo("Type", auto_type_bindings[static_cast<u32>(auto_type)])) {
                for (u32 i = 0; i < auto_type_bindings_count; i++) {
                    if (ImGui::Selectable(auto_type_bindings[i]))
                        auto_type = static_cast<assets::TileType>(i);
                    if (static_cast<assets::TileType>(i) == auto_type)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            static int input_tile_size = global_tile_size::get();
            bool can_modify_tile_size =
                detail::AssetContainer<assets::Tileset>::get_instance().map.empty();
            if (!can_modify_tile_size)
                input_tile_size = global_tile_size::get();
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
                valid &= global_tile_size::get() > 0 && global_tile_size::get() < 1024;
            }

            auto tex = preview_texture.get();
            if (tex) {
                if (tex->handle.width() % input_tile_size != 0 ||
                    tex->handle.height() % input_tile_size != 0) {
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
                if (auto_type == assets::TileType::rpgmaker_a2) {
                    if (tex->handle.width() % (2 * input_tile_size) != 0 ||
                        tex->handle.height() % (3 * input_tile_size) != 0) {
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

            if (auto tex = preview_texture.get()) {
                ImGui::BeginChild("preview_texture",
                                  {0, -ImGui::GetTextLineHeightWithSpacing() - 10});
                ImGui::Image(tex->handle.imgui_id(),
                             ImVec2{static_cast<float>(tex->handle.width()),
                                    static_cast<float>(tex->handle.height())});
                ImGui::EndChild();
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
                    global_tile_size::set(static_cast<std::size_t>(input_tile_size));

                assets::Tileset tileset;
                tileset.name = fs::path(path_selected).filename().generic_string();
                tileset.tile_type = auto_type;
                // We don't want to pass in preview_texture because it might be deleted any time
                tileset.texture = asset_manager::load<assets::TextureAsset>({path_selected});
                selection.tileset = asset_manager::put(tileset);
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
}

std::vector<TilesetSelection::Tile> TilesetSelection::tiles_selected() const {
    std::vector<TilesetSelection::Tile> sel;
    if (!selection.tileset.get())
        return sel;
    for (int y = selection_start.y; y <= selection_end.y; ++y) {
        for (int x = selection_start.x; x <= selection_end.x; ++x) {
            sel.emplace_back(TilesetSelection::Tile{
                {x, y},
                assets::Tileset::Tile{
                    selection.tileset,
                    static_cast<u64>(x + y * selection.tileset.get()->size_in_tile_units().x)}});
        }
    }
    return sel;
}
} // namespace arpiyi::tileset_manager