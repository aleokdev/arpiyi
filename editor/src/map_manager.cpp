#include "map_manager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "assets/entity.hpp"
#include "assets/texture.hpp"
#include "global_tile_size.hpp"
#include "tileset_manager.hpp"
#include "util/defs.hpp"
#include "util/icons_material_design.hpp"
#include "util/imgui_addons.hpp"
#include "widgets/inspector.hpp"
#include "widgets/pickers.hpp"
#include "window_list_menu.hpp"
#include "window_manager.hpp"

#include <anton/math/vector2.hpp>
#include <imgui.h>

namespace aml = anton::math;

namespace arpiyi::map_manager {

static Handle<assets::Map> current_map;
static Handle<assets::Map::Layer> current_layer_selected;
/// (In tiles).
/// TODO: Remove map_scroll & directly modify render map context
static aml::Vector2 map_scroll{0, 0};
static std::array<float, 5> zoom_levels = {.2f, .5f, 1.f, 2.f, 5.f};
static int current_zoom_level = 2;
static bool show_grid = true;
enum class EditMode { tile, comment, entity, height } edit_mode = EditMode::tile;
static float light_x_rotation = -M_PI / 5.f, light_z_rotation = -M_PI / 5.f;
static std::unique_ptr<renderer::RenderMapContext> render_map_context;

constexpr const char* map_view_strid = ICON_MD_TERRAIN " Map View";

static float get_map_zoom() { return zoom_levels[current_zoom_level]; }

static ImVec2 map_to_widget_pos(aml::Vector2 map_pos) {
    aml::Vector2 window_size{ImGui::GetContentRegionMax().x - ImGui::GetWindowContentRegionMin().x,
                             ImGui::GetContentRegionMax().y - ImGui::GetWindowContentRegionMin().y};
    aml::Vector2 result = window_size / 2.f -
                          aml::Vector2(current_map.get()->width, -current_map.get()->height) *
                              global_tile_size::get() * get_map_zoom() / 2.f +
                          aml::Vector2(map_pos.x + map_scroll.x, -map_pos.y - map_scroll.y) *
                              global_tile_size::get() * get_map_zoom();
    return ImVec2{aml::floor(result.x), aml::floor(result.y)};
};

static aml::Vector2 widget_to_map_tile_pos(ImVec2 widget_pos) {
    ImVec2 start_map_pos = map_to_widget_pos({0, 0});
    ImVec2 result{(widget_pos.x - start_map_pos.x) / global_tile_size::get() / get_map_zoom(),
                  (-widget_pos.y + start_map_pos.y) / global_tile_size::get() / get_map_zoom()};
    return {result.x, result.y};
};

static void show_info_tip(const char* c) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(c);
        ImGui::EndTooltip();
    }
}

static void show_add_layer_window(bool* p_open) {
    if (ImGui::Begin(ICON_MD_ADD_BOX " New Map Layer", p_open)) {
        static char name[32];
        static Handle<assets::Tileset> tileset;
        bool valid = tileset.get() && strlen(name) > 0;
        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the layer internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

        widgets::tileset_picker::show(tileset);

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("OK", valid)) {
            assets::Map::Layer layer{current_map.get()->width, current_map.get()->height, tileset};
            layer.name = name;
            current_map.get()->layers.emplace_back(asset_manager::put(layer));
            *p_open = false;
        }
    }
    ImGui::End();
}

static void show_edit_layer_window(bool* p_open, Handle<assets::Map::Layer> _l) {
    assert(_l.get());
    auto& layer = *_l.get();

    if (ImGui::Begin(ICON_MD_SETTINGS " Edit Map Layer", p_open)) {
        static char name[32];
        static Handle<assets::Tileset> tileset;
        if (ImGui::IsWindowAppearing()) {
            assert(layer.name.size() < 32);
            strcpy(name, layer.name.c_str());
            tileset = layer.tileset;
        }
        bool valid = tileset.get() && strlen(name) > 0;

        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the layer internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

        widgets::tileset_picker::show(tileset);

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("OK", valid)) {
            layer.name = name;
            layer.tileset = tileset;
            *p_open = false;
        }
    }
    ImGui::End();
}

static void show_add_map_window(bool* p_open) {
    ImGui::SetNextWindowSize({390, 190}, ImGuiCond_Once);
    if (ImGui::Begin(ICON_MD_ADD_BOX " New Map", p_open,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        static char name[32] = "Default";
        static i32 map_size[2] = {16, 16};
        static bool create_default_layer = true;
        static char layer_name[32] = "Base Layer";
        static Handle<assets::Tileset> layer_tileset = Handle<assets::Tileset>::noid;
        bool valid = true;

        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the map internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

        ImGui::InputInt2("Map Size", map_size);
        show_info_tip("The map size (In tiles).");
        valid &= map_size[0] > 0 && map_size[1] > 0;

        ImGui::Checkbox("Create default layer", &create_default_layer);
        if (create_default_layer) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
            ImGui::BeginChild("Default layer options", {0, -ImGui::GetTextLineHeightWithSpacing()},
                              true, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::InputText("Name", layer_name, 32);
            widgets::tileset_picker::show(layer_tileset);
            ImGui::EndChild();
            ImGui::PopStyleVar();
            valid &= layer_tileset.get();
        }

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("OK", valid)) {
            assets::Map map;
            map.width = static_cast<decltype(map.width)>(map_size[0]);
            map.height = static_cast<decltype(map.height)>(map_size[1]);
            map.name = name;
            if (create_default_layer) {
                map.layers.emplace_back(
                    asset_manager::put(assets::Map::Layer(map.width, map.height, layer_tileset)));
                map.layers[0].get()->name = layer_name;
                current_layer_selected = map.layers[0];
            }
            current_map = asset_manager::put<assets::Map>(map);
            *p_open = false;
        }
    }
    ImGui::End();
}

static void show_edit_map_window(bool* p_open, Handle<assets::Map> _m) {
    assert(_m.get());
    auto& map = *_m.get();
    ImGui::SetNextWindowSize({390, 190}, ImGuiCond_Once);
    if (ImGui::Begin(ICON_MD_SETTINGS " Edit Map", p_open,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        static char name[32];
        if (ImGui::IsWindowAppearing()) {
            strcpy(name, map.name.c_str());
        }

        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the map internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("OK")) {
            map.name = name;
            *p_open = false;
        }
    }
    ImGui::End();
}

static void show_map_entity_list() {
    if (ImGui::Begin(ICON_MD_VIDEOGAME_ASSET " Map Entities###m_edit_panel", nullptr,
                     ImGuiWindowFlags_MenuBar)) {
        if (auto map = current_map.get()) {
            for (const auto& e : map->entities) {
                auto& entity = *e.get();
                ImGui::TextDisabled("%zu", e.get_id());
                ImGui::SameLine();
                ImGui::TextUnformatted(entity.name.c_str());
            }
        }
    }
    ImGui::End();
}

static void show_map_layer_list() {
    static bool show_add_layer = false;
    static Handle<assets::Map::Layer> layer_being_edited;

    auto map = current_map.get();
    if (ImGui::Begin(ICON_MD_LAYERS " Map Layers###m_edit_panel", nullptr,
                     ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...", nullptr, nullptr, map)) {
                show_add_layer = true;
            }
            ImGui::EndMenuBar();
        }

        if (map) {
            if (map->layers.empty())
                ImGui::TextDisabled("No layers in current map");
            else {
                Handle<assets::Map::Layer> layer_to_delete = -1;
                for (auto& l : map->layers) {
                    auto& layer = *l.get();
                    ImGui::TextDisabled("%zu", l.get_id());
                    ImGui::SameLine();
                    if (ImGui::TextDisabled("%s", layer.visible ? ICON_MD_VISIBILITY
                                                                : ICON_MD_VISIBILITY_OFF),
                        ImGui::IsItemClicked()) {
                        layer.visible = !layer.visible;
                    }
                    ImGui::SameLine();
                    if (ImGui::Selectable(layer.name.c_str(), l == current_layer_selected),
                        ImGui::IsItemClicked()) {
                        ImGui::SetWindowFocus(map_view_strid);
                        current_layer_selected = l;
                        tileset_manager::set_selection_tileset(layer.tileset);
                    }

                    {
                        char buf[32];
                        sprintf(buf, "del_%zu", l.get_id());
                        if (ImGui::BeginPopupContextItem(buf)) {
                            if (ImGui::Selectable("Edit...")) {
                                layer_being_edited = l;
                            }
                            if (ImGui::Selectable("Delete"))
                                layer_to_delete = l;
                            ImGui::EndPopup();
                        }
                    }
                }

                if (layer_to_delete.get_id() != Handle<assets::Map::Layer>::noid) {
                    map->layers.erase(
                        std::remove(map->layers.begin(), map->layers.end(), layer_to_delete),
                        map->layers.end());
                    layer_to_delete.unload();
                }
            }
        } else {
            ImGui::TextDisabled("No map selected");
        }
    }
    ImGui::End();

    if (show_add_layer) {
        show_add_layer_window(&show_add_layer);
    }
    if (layer_being_edited.get()) {
        bool p_open = true;
        show_edit_layer_window(&p_open, layer_being_edited);
        if (!p_open)
            layer_being_edited = nullptr;
    }
}

static void show_map_list() {
    static bool show_add_map = false;
    static Handle<assets::Map> map_being_edited;
    if (ImGui::Begin(ICON_MD_VIEW_LIST " Map List##map_list", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...")) {
                show_add_map = true;
            }
            ImGui::EndMenuBar();
        }
        if (detail::AssetContainer<assets::Map>::get_instance().map.empty())
            ImGui::TextDisabled("No maps");
        else
            for (auto& [_id, _m] : detail::AssetContainer<assets::Map>::get_instance().map) {
                ImGui::TextDisabled("%zu", _id);
                ImGui::SameLine();
                if (ImGui::Selectable(_m.name.c_str(), _id == current_map.get_id())) {
                    ImGui::SetWindowFocus(map_view_strid);
                    current_map = Handle<assets::Map>(_id);
                    if (!current_map.get()->layers.empty()) {
                        current_layer_selected = current_map.get()->layers[0];
                        tileset_manager::set_selection_tileset(
                            current_layer_selected.get()->tileset);
                    }
                }
                if (ImGui::BeginPopupContextWindow(std::to_string(_id).c_str())) {
                    if (ImGui::Selectable("Edit...")) {
                        map_being_edited = Handle<assets::Map>(_id);
                    }
                    ImGui::EndPopup();
                }
            }
    }
    ImGui::End();

    if (show_add_map) {
        show_add_map_window(&show_add_map);
    }
    if (map_being_edited.get()) {
        bool p_open = true;
        show_edit_map_window(&p_open, map_being_edited);
        if (!p_open)
            map_being_edited = nullptr;
    }
}

static void draw_pos_info_bar() {
    ImVec2 info_rect_start{ImGui::GetWindowPos().x, ImGui::GetWindowPos().y +
                                                        ImGui::GetWindowSize().y -
                                                        ImGui::GetTextLineHeightWithSpacing()};
    ImVec2 info_rect_end{ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
                         ImGui::GetWindowPos().y + ImGui::GetWindowSize().y};
    ImGui::GetWindowDrawList()->PushClipRectFullScreen();
    ImGui::GetWindowDrawList()->AddRectFilled(info_rect_start, info_rect_end,
                                              ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    ImGui::GetWindowDrawList()->PopClipRect();
    ImVec2 text_pos{ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x,
                    info_rect_start.y};
    {
        ImVec2 map_start_widget_pos = map_to_widget_pos({0, 0});
        ImVec2 map_end_widget_pos =
            map_to_widget_pos({static_cast<float>(current_map.get()->width),
                               static_cast<float>(current_map.get()->height)});
        aml::Vector2 tile_pos =
            widget_to_map_tile_pos({ImGui::GetMousePos().x - ImGui::GetWindowPos().x -
                                        ImGui::GetWindowContentRegionMin().x,
                                    ImGui::GetMousePos().y - ImGui::GetWindowPos().y -
                                        ImGui::GetWindowContentRegionMin().y});
        char buf[256];
        sprintf(buf, "Tile pos: {%i, %i} - Zoom: %i%% - MSWP: {%.2f, %.2f} - MEWP: {%.2f, %.2f}",
                static_cast<int>(tile_pos.x), static_cast<int>(tile_pos.y),
                static_cast<i32>(get_map_zoom() * 10.f) * 10, map_start_widget_pos.x,
                map_start_widget_pos.y, map_end_widget_pos.x, map_end_widget_pos.y);
        ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), buf);
    }
}

static void resize_map_fb(int width, int height) {
    render_map_context->output_fb.set_size({width, height});
    render_map_context->set_shadow_resolution({width, height});
}

static void render_map() {
    if (auto m = current_map.get()) {
        render_map_context->draw_grid = show_grid;
        render_map_context->x_light_rotation = light_x_rotation;
        render_map_context->z_light_rotation = light_z_rotation;
        render_map_context->zoom = get_map_zoom();
        render_map_context->cam_pos = map_scroll;
        render_map_context->map = current_map;
        window_manager::get_renderer().draw_map(*render_map_context);
    }
}

static void place_tile_on_pos(assets::Map& map,
                              math::IVec2D pos,
                              bool update_neighbours_of_different_type = true,
                              bool join_with_neighbours_of_different_type = false) {
    if (!(pos.x >= 0 && pos.y >= 0 && pos.x < map.width && pos.y < map.height))
        return;

    const auto selection = tileset_manager::get_selection();
    assert(current_layer_selected.get());
    auto& layer = *current_layer_selected.get();

    switch (selection.tileset.get()->auto_type) {
        case (assets::Tileset::AutoType::none): {
            int start_my = pos.y;
            for (int tx = selection.selection_start.x; tx <= selection.selection_end.x; tx++) {
                for (int ty = selection.selection_start.y; ty <= selection.selection_end.y; ty++) {
                    if (!(pos.x >= 0 && pos.y >= 0 && pos.x < map.width && pos.y < map.height))
                        continue;
                    layer.get_tile(pos).id = layer.tileset.get()->get_id_autotype_none({tx, ty});
                    pos.y--;
                }
                pos.x++;
                pos.y = start_my;
            }
        } break;

        case (assets::Tileset::AutoType::rpgmaker_a2): {
            const auto& tileset = *selection.tileset.get();
            const auto are_tiles_of_same_type = [&tileset](u32 id1, u32 id2) -> bool {
                return tileset.get_auto_tile_index_from_auto_id(id1) ==
                       tileset.get_auto_tile_index_from_auto_id(id2);
            };
            const auto selected_auto_tile = tileset.get_id_autotype_rpgmaker_a2(selection.selection_start.x, 0);

            const auto update_auto_id = [&](math::IVec2D pos) {
                u8 surroundings = 0xFF;
                u8 bit = 0;
                const assets::Map::Tile self_tile = layer.get_tile(pos);
                for (int iy = -1; iy <= 1; ++iy) {
                    for (int ix = -1; ix <= 1; ++ix) {
                        if (ix == 0 && iy == 0)
                            continue;
                        math::IVec2D neighbour_pos{pos.x + ix, pos.y - iy};
                        if (!layer.is_pos_valid(neighbour_pos)) {
                            bit++;
                            continue;
                        }
                        assets::Map::Tile neighbour = layer.get_tile(neighbour_pos);
                        bool is_neighbour_of_same_type;
                        if (join_with_neighbours_of_different_type)
                            is_neighbour_of_same_type =
                                are_tiles_of_same_type(neighbour.id, self_tile.id) ||
                                are_tiles_of_same_type(neighbour.id, selected_auto_tile);
                        else
                            is_neighbour_of_same_type =
                                are_tiles_of_same_type(neighbour.id, self_tile.id);
                        surroundings ^= is_neighbour_of_same_type << bit;
                        bit++;
                    }
                }
                layer.get_tile(pos).id = tileset.get_id_autotype_rpgmaker_a2(
                    tileset.get_auto_tile_index_from_auto_id(self_tile.id), surroundings);
            };
            // Set the tile below the cursor and don't worry about the surroundings; we'll update
            // them later
            layer.get_tile(pos).id =selected_auto_tile;
            // Update autoID of tile placed and all others near it
            for (int iy = -1; iy <= 1; ++iy) {
                for (int ix = -1; ix <= 1; ++ix) {
                    const math::IVec2D ipos = {pos.x + ix, pos.y + iy};
                    if (!layer.is_pos_valid(ipos))
                        continue;
                    if (!update_neighbours_of_different_type &&
                        !are_tiles_of_same_type(layer.get_tile(pos).id, layer.get_tile(ipos).id))
                        continue;
                    update_auto_id(ipos);
                }
            }
        } break;

        default: ARPIYI_UNREACHABLE(); break;
    }

    layer.regenerate_mesh();
}

static void draw_selection_on_map(assets::Map& map, bool is_tileset_appropiate_for_layer) {
    auto selection = tileset_manager::get_selection();
    if (auto selection_tileset = selection.tileset.get()) {
        ImVec2 map_selection_size{
            (float)(selection.selection_end.x + 1 - selection.selection_start.x) *
                global_tile_size::get() * get_map_zoom(),
            (float)(selection.selection_end.y + 1 - selection.selection_start.y) *
                global_tile_size::get() * get_map_zoom()};
        ImVec2 mouse_pos{ImGui::GetMousePos().x, ImGui::GetMousePos().y - map_selection_size.y};
        ImVec2 relative_mouse_pos{ImGui::GetMousePos().x - ImGui::GetWindowPos().x -
                                      ImGui::GetWindowContentRegionMin().x,
                                  ImGui::GetMousePos().y - ImGui::GetWindowPos().y -
                                      ImGui::GetWindowContentRegionMin().y};
        math::IVec2D tileset_size = selection_tileset->get_size_in_tiles();
        ImVec2 uv_min = ImVec2{(float)selection.selection_start.x / (float)tileset_size.x,
                               (float)selection.selection_start.y / (float)tileset_size.y};
        ImVec2 uv_max = ImVec2{(float)(selection.selection_end.x + 1) / (float)tileset_size.x,
                               (float)(selection.selection_end.y + 1) / (float)tileset_size.y};
        ImVec2 clip_rect_min = map_to_widget_pos({0, 0});
        clip_rect_min.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
        clip_rect_min.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
        ImVec2 clip_rect_max =
            map_to_widget_pos({static_cast<float>(map.width), static_cast<float>(map.height)});
        clip_rect_max.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
        clip_rect_max.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
        aml::Vector2 mouse_tile_pos = widget_to_map_tile_pos(relative_mouse_pos);
        ImVec2 mouse_tile_widget_pos =
            map_to_widget_pos({std::floor(mouse_tile_pos.x), std::ceil(mouse_tile_pos.y)});
        ImVec2 image_rect_min = {mouse_tile_widget_pos.x + ImGui::GetWindowPos().x +
                                     ImGui::GetWindowContentRegionMin().x,
                                 mouse_tile_widget_pos.y + ImGui::GetWindowPos().y +
                                     ImGui::GetWindowContentRegionMin().y};
        ImVec2 image_rect_max = ImVec2{image_rect_min.x + map_selection_size.x,
                                       image_rect_min.y + map_selection_size.y};

        ImGui::GetWindowDrawList()->PushClipRect({clip_rect_min.x, clip_rect_max.y},
                                                 {clip_rect_max.x, clip_rect_min.y}, true);
        ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<ImTextureID>(selection_tileset->texture.get()->handle), image_rect_min,
            image_rect_max, uv_min, uv_max, ImGui::GetColorU32({1.f, 1.f, 1.f, 0.4f}));
        ImGui::GetWindowDrawList()->PopClipRect();
    }
}

/// @returns One of the entities below the cursor (if any)
static Handle<assets::Entity> draw_entities(const assets::Map& map) {
    Handle<assets::Entity> entity_hovering;
    for (const auto& e : map.entities) {
        assert(e.get());
        const auto& entity = *e.get();
        const math::IVec2D entity_sprite_size =
            entity.sprite.get() ? entity.sprite.get()->get_size_in_pixels()
                                : math::IVec2D{static_cast<i32>(global_tile_size::get()),
                                               static_cast<i32>(global_tile_size::get())};

        const auto widget_to_absolute_window_pos = [](ImVec2 pos) -> ImVec2 {
            return {pos.x + ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x,
                    pos.y + ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y};
        };

        const aml::Vector2 tile_entity_square_render_pos_min = entity.get_origin_pos();
        ImVec2 entity_square_render_pos_min =
            widget_to_absolute_window_pos(map_to_widget_pos(tile_entity_square_render_pos_min));
        aml::Vector2 entity_square_render_pos_max =
            aml::Vector2(entity_square_render_pos_min.x, entity_square_render_pos_min.y) +
            aml::Vector2{static_cast<float>(entity_sprite_size.x) * get_map_zoom(),
                         -static_cast<float>(entity_sprite_size.y) * get_map_zoom()};

        if (edit_mode == EditMode::entity) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                {entity_square_render_pos_min.x, entity_square_render_pos_max.y},
                {entity_square_render_pos_max.x, entity_square_render_pos_min.y},
                ImGui::GetColorU32({0.1f, 0.8f, 0.9f, 0.6f}));
        }
        if (ImGui::IsMouseHoveringRect(
                {entity_square_render_pos_min.x, entity_square_render_pos_max.y},
                {entity_square_render_pos_max.x, entity_square_render_pos_min.y})) {
            entity_hovering = e;
            if (edit_mode == EditMode::entity) {
                ImGui::BeginTooltip();
                ImGui::Text("%s at {%.2f, %.2f}", entity.name.c_str(), entity.pos.x, entity.pos.y);
                ImGui::Separator();
                ImGui::TextDisabled("Scripts:");
                ImGui::BeginChild("##ent_scripts");
                for (const auto& script : entity.scripts) {
                    assert(script.get());
                    ImGui::TextUnformatted(script.get()->name.c_str());
                }
                ImGui::EndChild();
                ImGui::EndTooltip();
            }
        }
    }
    return entity_hovering;
}

/// @returns One of the comments below the cursor (if any)
static Handle<assets::Map::Comment> draw_comments(assets::Map const& map) {
    // FIXME this uses the old coordinate system
    Handle<assets::Map::Comment> comment_hovering;
    for (const auto& c : map.comments) {
        assert(c.get());
        const auto& comment = *c.get();
        ImVec2 comment_square_render_pos_min = map_to_widget_pos(
            {static_cast<float>(comment.pos.x), static_cast<float>(comment.pos.y)});
        ImVec2 comment_square_render_pos_max = {
            comment_square_render_pos_min.x + global_tile_size::get() * get_map_zoom(),
            comment_square_render_pos_min.y + global_tile_size::get() * get_map_zoom()};
        ImGui::GetWindowDrawList()->AddRect(
            comment_square_render_pos_min, comment_square_render_pos_max,
            ImGui::GetColorU32({0.9f, 0.8f, 0.05f, 0.6f}), 0, ImDrawCornerFlags_All, 5.f);
        if (ImGui::IsMouseHoveringRect(comment_square_render_pos_min,
                                       comment_square_render_pos_max)) {
            comment_hovering = c;
            ImGui::BeginTooltip();
            ImGui::TextDisabled("Text comment at {%i, %i}", comment.pos.x, comment.pos.y);
            ImGui::Separator();
            ImGui::TextUnformatted(comment.text.c_str());
            ImGui::EndTooltip();
        }
    }

    return comment_hovering;
}

static void process_map_input(assets::Map& map,
                              Handle<assets::Entity>& entity_hovering,
                              Handle<assets::Map::Comment>& comment_hovering) {
    ImVec2 window_mouse_pos{
        ImGui::GetMousePos().x - ImGui::GetWindowPos().x - ImGui::GetWindowContentRegionMin().x,
        ImGui::GetMousePos().y - ImGui::GetWindowPos().y - ImGui::GetWindowContentRegionMin().y};
    aml::Vector2 mouse_tile_pos = widget_to_map_tile_pos(window_mouse_pos);
    // Process middle click input (Camera moving)
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseDown[ImGuiMouseButton_Middle]) {
        ImGui::SetWindowFocus();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        map_scroll =
            aml::Vector2{map_scroll.x + io.MouseDelta.x / global_tile_size::get() / get_map_zoom(),
                         map_scroll.y - io.MouseDelta.y / global_tile_size::get() / get_map_zoom()};
    }
    switch (edit_mode) {
        case EditMode::comment: {
            ImGuiIO& io = ImGui::GetIO();
            if (!comment_hovering.get() && io.MouseDoubleClicked[ImGuiMouseButton_Left]) {
                if (auto layer = current_layer_selected.get()) {
                    math::IVec2D pos{static_cast<int>(mouse_tile_pos.x),
                                     static_cast<int>(mouse_tile_pos.y)};
                    if (layer->is_pos_valid(pos)) {
                        assets::Map::Comment comment;
                        comment.pos = pos;

                        map.comments.emplace_back(asset_manager::put(comment));
                    }
                }
            } else if (auto comment = comment_hovering.get()) {
                if (io.MouseClicked[ImGuiMouseButton_Left])
                    widgets::inspector::set_inspected_asset(comment_hovering);

                if (io.MouseDown[ImGuiMouseButton_Left]) {
                    comment->pos.x = static_cast<float>(mouse_tile_pos.x);
                    comment->pos.y = static_cast<float>(mouse_tile_pos.y);
                } else {
                    comment_hovering = nullptr;
                }
            }
        } break;

        case EditMode::entity: {
            ImGuiIO& io = ImGui::GetIO();
            math::IVec2D pos{static_cast<int>(mouse_tile_pos.x),
                             static_cast<int>(mouse_tile_pos.y)};
            if (!entity_hovering.get() && io.MouseDoubleClicked[ImGuiMouseButton_Left]) {
                if (auto layer = current_layer_selected.get()) {
                    if (layer->is_pos_valid(pos)) {
                        assets::Entity entity;
                        entity.name = "Entity";
                        if (io.KeyCtrl) {
                            entity.pos = {static_cast<float>(pos.x), static_cast<float>(pos.y)};
                        } else {
                            entity.pos = mouse_tile_pos;
                        }
                        map.entities.emplace_back(asset_manager::put(entity));
                    }
                }
            } else if (auto entity = entity_hovering.get()) {
                if (io.MouseClicked[ImGuiMouseButton_Left])
                    widgets::inspector::set_inspected_asset(entity_hovering);

                if (io.MouseDown[ImGuiMouseButton_Left]) {
                    if (io.KeyCtrl) {
                        entity->pos.x = static_cast<float>(pos.x);
                        entity->pos.y = static_cast<float>(pos.y);
                    } else {
                        entity->pos = mouse_tile_pos;
                    }
                } else {
                    entity_hovering = nullptr;
                }
            }
        } break;

        case EditMode::tile: {
            ImVec2 map_render_min = map_to_widget_pos({0, 0});
            map_render_min.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
            map_render_min.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
            ImVec2 map_render_max =
                map_to_widget_pos({static_cast<float>(map.width), static_cast<float>(map.height)});
            map_render_max.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
            map_render_max.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
            if (ImGui::IsMouseHoveringRect({map_render_min.x, map_render_max.y},
                                           {map_render_max.x, map_render_min.y}) &&
                ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                if (auto layer = current_layer_selected.get()) {
                    math::IVec2D pos{static_cast<int>(mouse_tile_pos.x),
                                     static_cast<int>(mouse_tile_pos.y)};
                    if (layer->is_pos_valid(pos)) {
                        place_tile_on_pos(map, pos, !ImGui::GetIO().KeyShift,
                                          ImGui::GetIO().KeyCtrl);
                    }
                }
            }
        } break;

        case EditMode::height: {
            ImVec2 map_render_min = map_to_widget_pos({0, 0});
            map_render_min.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
            map_render_min.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
            ImVec2 map_render_max =
                map_to_widget_pos({static_cast<float>(map.width), static_cast<float>(map.height)});
            map_render_max.x += ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
            map_render_max.y += ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y;
            if (ImGui::IsMouseHoveringRect({map_render_min.x, map_render_max.y},
                                           {map_render_max.x, map_render_min.y})) {
                if (auto layer = current_layer_selected.get()) {
                    math::IVec2D pos{static_cast<int>(mouse_tile_pos.x),
                                     static_cast<int>(mouse_tile_pos.y)};
                    if (layer->is_pos_valid(pos)) {
                        ImGuiIO& io = ImGui::GetIO();
                        assets::Map::Tile& t = layer->get_tile(pos);
                        if (io.KeyMods & ImGuiKeyModFlags_Shift &&
                            io.MouseClicked[ImGuiMouseButton_Left]) {
                            t.slope_type = (assets::Map::Tile::SlopeType)((u8)(t.slope_type) + 1);
                            if (t.slope_type == assets::Map::Tile::SlopeType::count)
                                t.slope_type = assets::Map::Tile::SlopeType::none;
                            layer->regenerate_mesh();
                        } else if (io.KeyMods & ImGuiKeyModFlags_Ctrl &&
                                   io.MouseClicked[ImGuiMouseButton_Left]) {
                            t.has_side_walls = !t.has_side_walls;
                            layer->regenerate_mesh();
                        } else if (io.MouseClicked[ImGuiMouseButton_Left]) {
                            t.height++;
                            layer->regenerate_mesh();
                        } else if (io.MouseClicked[ImGuiMouseButton_Right]) {
                            t.height--;
                            layer->regenerate_mesh();
                        }
                    }
                }
            }
        } break;
    }
}

void init() {
    render_map_context = std::make_unique<renderer::RenderMapContext>(math::IVec2D{1, 1});
    window_list_menu::add_entry({"Map View", &render});
}

void render(bool* p_show) {
    auto map = current_map.get();

    if (ImGui::Begin(map_view_strid, p_show,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
                         ImGuiWindowFlags_NoScrollWithMouse)) {
        if (map) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Checkbox("Grid", &show_grid);

                const auto draw_edit_mode = [](EditMode mode, const char* icon,
                                               const char* tooltip) {
                    if (edit_mode != mode)
                        ImGui::PushStyleColor(ImGuiCol_Text, {0.6f, 0.6f, 0.6f, 1.f});
                    if (ImGui::MenuItem(icon, nullptr) && edit_mode != mode) {
                        edit_mode = mode;
                        ImGui::PopStyleColor();
                    }
                    if (edit_mode != mode)
                        ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", tooltip);
                    }
                };

                draw_edit_mode(
                    EditMode::tile, ICON_MD_TERRAIN,
                    "Terrain editing tool.\nUse tiles to change the appearance of your map.");
                draw_edit_mode(EditMode::comment, ICON_MD_COMMENT,
                               "Comment editing tool.\nUse text comments to annotate things on "
                               "your map.\nThese won't have an impact on the actual game.");
                draw_edit_mode(EditMode::entity, ICON_MD_VIDEOGAME_ASSET,
                               "Entity editing tool.\nUse entities to give life to your maps.");
                draw_edit_mode(
                    EditMode::height, ICON_MD_FLIP_TO_BACK,
                    "Height editing tool.\nUse the height brush to create realistic shadows.");

                ImGui::EndMenuBar();
            }

            if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered()) {
                if (ImGui::GetIO().MouseWheel > 0 &&
                    current_zoom_level < static_cast<i32>(zoom_levels.size()) - 1)
                    current_zoom_level += 1;
                else if (ImGui::GetIO().MouseWheel < 0 && current_zoom_level > 0)
                    current_zoom_level -= 1;
            }

            ImVec2 abs_content_start_pos = {
                ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x,
                ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y};
            {
                static ImVec2 last_window_size;
                // Draw the map
                if (ImGui::GetWindowSize().x != last_window_size.x ||
                    ImGui::GetWindowSize().y != last_window_size.y) {
                    resize_map_fb(static_cast<int>(ImGui::GetWindowContentRegionWidth()),
                                  static_cast<int>(ImGui::GetWindowContentRegionMax().y -
                                                   ImGui::GetWindowContentRegionMin().y));
                    last_window_size = ImGui::GetWindowSize();
                }
                render_map();
                const auto& fb = render_map_context->output_fb;
                ImGui::Image(
                    fb.get_imgui_id(),
                    {static_cast<float>(fb.get_size().x), static_cast<float>(fb.get_size().y)},
                    {0, 1}, {1, 0});
            }

            bool is_tileset_appropiate_for_layer;
            if (auto layer = current_layer_selected.get())
                is_tileset_appropiate_for_layer =
                    tileset_manager::get_selection().tileset == layer->tileset;
            else
                is_tileset_appropiate_for_layer = true;

            if (edit_mode == EditMode::tile) {
                draw_selection_on_map(*map, is_tileset_appropiate_for_layer);
            }

            static Handle<assets::Entity> entity_hovering;
            if (!entity_hovering.get()) {
                entity_hovering = draw_entities(*map);
            } else {
                draw_entities(*map);
            }

            static Handle<assets::Map::Comment> comment_hovering;
            if (!comment_hovering.get()) {
                comment_hovering = draw_comments(*map);
            } else {
                draw_comments(*map);
            }

            if (is_tileset_appropiate_for_layer) {
                process_map_input(*map, entity_hovering, comment_hovering);
            }

            if (!is_tileset_appropiate_for_layer) {
                constexpr std::string_view text =
                    "Tileset does not correspond the one specified in the selected layer.";
                ImVec2 text_size = ImGui::CalcTextSize(text.data());
                ImVec2 text_pos{
                    ImGui::GetWindowPos().x + ImGui::GetWindowWidth() / 2.f - text_size.x / 2.f,
                    ImGui::GetWindowPos().y + ImGui::GetWindowHeight() / 2.f - text_size.y / 2.f};
                // Draw black rect on top to darken the already drawn stuff.
                ImGui::GetWindowDrawList()->AddRectFilled(
                    {0, 0},
                    {ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
                     ImGui::GetWindowPos().y + ImGui::GetWindowSize().y},
                    ImGui::GetColorU32({0, 0, 0, .4f}));
                ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text),
                                                    text.data());
            }

            draw_pos_info_bar();
        } else
            ImGui::TextDisabled("No map loaded to view.");
    }
    ImGui::End();

    if (ImGui::Begin("Light Settings")) {
        ImGui::SliderFloat("X Rotation", &light_x_rotation, -M_PI, M_PI);
        ImGui::SliderFloat("Z Rotation", &light_z_rotation, -M_PI, M_PI);
    }
    ImGui::End();

    switch (edit_mode) {
        case EditMode::entity: show_map_entity_list(); break;

        case EditMode::tile: show_map_layer_list(); break;

        default: break;
    }

    show_map_list();
} // namespace arpiyi_editor::map_manager

Handle<assets::Map> get_current_map() { return current_map; }

std::vector<Handle<assets::Map>> get_maps() {
    std::vector<Handle<assets::Map>> maps;
    for (const auto& [id, map] : arpiyi::detail::AssetContainer<assets::Map>::get_instance().map) {
        maps.emplace_back(id);
    }
    return maps;
}

} // namespace arpiyi::map_manager