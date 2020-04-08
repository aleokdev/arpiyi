#include "map_manager.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "assets/shader.hpp"
#include "assets/texture.hpp"
#include "editor/editor_style.hpp"
#include "tileset_manager.hpp"
#include "util/defs.hpp"
#include "util/icons_material_design.hpp"
#include "window_manager.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace arpiyi_editor::map_manager {

Handle<assets::Map> current_map;
Handle<assets::Shader> tile_shader;
Handle<assets::Shader> grid_shader;
Handle<assets::Mesh> quad_mesh;
glm::mat4 proj_mat;
Handle<assets::Map::Layer> current_layer_selected;
ImVec2 map_scroll{0, 0};
std::array<float, 5> zoom_levels = {.2f, .5f, 1.f, 2.f, 5.f};
int current_zoom_level = 2;
static bool show_grid = true;

static float get_map_zoom() { return zoom_levels[current_zoom_level]; }

static void show_add_layer_window(bool* p_open) {
    if (ImGui::Begin(ICON_MD_ADD_BOX " New Map Layer", p_open)) {
        static char name[32];
        static Handle<assets::Tileset> tileset;
        const auto& show_info_tip = [](const char* c) {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(c);
                ImGui::EndTooltip();
            }
        };
        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the layer internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

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

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (!t) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK")) {
            assets::Map::Layer layer{current_map.get()->width, current_map.get()->height, tileset};
            layer.name = name;
            current_map.get()->layers.emplace_back(asset_manager::put(layer));
            *p_open = false;
        }
        if (!t) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
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
        const auto& show_info_tip = [](const char* c) {
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(c);
                ImGui::EndTooltip();
            }
        };
        ImGui::InputText("Internal Name", name, 32);
        show_info_tip("The name given to the map internally. This won't appear in the "
                      "game unless you specify so. Can be changed later.");

        ImGui::InputInt2("Map Size", map_size);
        show_info_tip("The map size (In tiles). Can be changed later.");

        ImGui::Checkbox("Create default layer", &create_default_layer);
        if (create_default_layer) {
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
            ImGui::BeginChild("Default layer options", {0, -ImGui::GetTextLineHeightWithSpacing()},
                              true, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::InputText("Name", layer_name, 32);
            auto t = layer_tileset.get();
            if (ImGui::BeginCombo("Tileset", t ? t->name.c_str() : "<Not selected>")) {
                for (auto& ts : tileset_manager::get_tilesets()) {
                    if (ImGui::Selectable(ts.get()->name.c_str(), ts == layer_tileset))
                        layer_tileset = ts;

                    if (ts == layer_tileset)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        bool allow_ok = !create_default_layer || layer_tileset.get();
        if (!allow_ok) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK")) {
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
        if (!allow_ok) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
    }
    ImGui::End();
}

static void draw_pos_info_bar(math::IVec2D tile_pos, ImVec2 relative_mouse_pos) {
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
        math::IVec2D mpos{static_cast<i32>(relative_mouse_pos.x),
                          static_cast<i32>(relative_mouse_pos.y)};
        char buf[128];
        sprintf(buf, "Tile pos: {%i, %i} - Mouse pos: {%i, %i} - Zoom: %i%%", tile_pos.x,
                tile_pos.y, mpos.x, mpos.y, static_cast<i32>(get_map_zoom() * 10.f) * 10);
        ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), buf);
    }
}

struct DrawMapCallbackData {
    ImVec2 map_render_pos;
    ImVec2 abs_content_min_rect;
};
static void draw_map_callback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
    auto fb_size = window_manager::get_framebuf_size();
    const auto callback_data = *static_cast<DrawMapCallbackData*>(cmd->UserCallbackData);
    glScissor(callback_data.abs_content_min_rect.x, fb_size.y - cmd->ClipRect.w,
              cmd->ClipRect.z - callback_data.abs_content_min_rect.x,
              cmd->ClipRect.w - callback_data.abs_content_min_rect.y);

    const auto& map = *current_map.get();
    // Calculate model matrix: This is the same for the grid and all layers, so we'll calculate it
    // first.
    float map_total_width = map.width * tileset_manager::get_tile_size() * get_map_zoom();
    float map_total_height = map.height * tileset_manager::get_tile_size() * get_map_zoom();

    float clip_rect_width = cmd->ClipRect.z - callback_data.abs_content_min_rect.x;
    float clip_rect_height = cmd->ClipRect.w - callback_data.abs_content_min_rect.y;
    glViewport(callback_data.abs_content_min_rect.x, fb_size.y - cmd->ClipRect.w, clip_rect_width,
               clip_rect_height);
    glm::mat4 model = glm::mat4(1);
    model = glm::translate(model, glm::vec3(0, map_total_height / clip_rect_height,
                                            0)); // Put model in left-top corner
    model = glm::translate(model, glm::vec3(callback_data.map_render_pos.x / clip_rect_width,
                                            callback_data.map_render_pos.y / clip_rect_height,
                                            0));    // Put model in given position
    model = glm::scale(model, glm::vec3(1, -1, 1)); // Flip model from its Y axis
    model = glm::scale(model, glm::vec3(map_total_width / clip_rect_width,
                                        map_total_height / clip_rect_height, 1));

    if (!map.layers.empty()) {
        glUseProgram(tile_shader.get()->handle);
        glActiveTexture(GL_TEXTURE0);
        // Draw each layer
        for (auto& _l : current_map.get()->layers) {
            auto layer = _l.get();
            if (!layer->visible)
                continue;

            glBindVertexArray(layer->get_mesh().get()->vao);
            glBindTexture(GL_TEXTURE_2D, layer->tileset.get()->texture.get()->handle);

            glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

            constexpr int quad_verts = 2 * 3;
            glDrawArrays(GL_TRIANGLES, 0, map.width * map.height * quad_verts);
        }
    }
    if (show_grid) {
        // Draw mesh grid
        glUseProgram(grid_shader.get()->handle);
        glBindVertexArray(quad_mesh.get()->vao);
        glUniform4f(3, .9f, .9f, .9f, .4f); // Grid color
        glUniform2ui(4, current_map.get()->width, current_map.get()->height);

        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);
    }
}

static void place_tile_on_pos(assets::Map& map, math::IVec2D pos) {
    if (!(pos.x >= 0 && pos.y >= 0 && pos.x < map.width && pos.y < map.height))
        return;

    const auto selection = tileset_manager::get_selection();

    switch (selection.tileset.get()->auto_type) {
        case (assets::Tileset::AutoType::none): {
            int start_my = pos.y;
            for (int tx = selection.selection_start.x; tx <= selection.selection_end.x; tx++) {
                for (int ty = selection.selection_start.y; ty <= selection.selection_end.y; ty++) {
                    if (!(pos.x >= 0 && pos.y >= 0 && pos.x < map.width && pos.y < map.height))
                        continue;
                    auto layer = current_layer_selected.get();
                    layer->set_tile(pos, {layer->tileset.get()->get_id({tx, ty})});
                    pos.y++;
                }
                pos.x++;
                pos.y = start_my;
            }
        } break;

        case (assets::Tileset::AutoType::rpgmaker_a2): {
            auto& layer = *current_layer_selected.get();
            const auto& tileset = *selection.tileset.get();
            const auto update_auto_id = [&layer, &tileset, &selection](math::IVec2D pos) {
                u8 surroundings = 0xFF;
                u8 bit = 0;
                const assets::Map::Tile self_tile = layer.get_tile(pos);
                for (int iy = -1; iy <= 1; ++iy) {
                    for (int ix = -1; ix <= 1; ++ix) {
                        if (ix == 0 && iy == 0)
                            continue;
                        math::IVec2D neighbour_pos{pos.x + ix, pos.y + iy};
                        if (!layer.is_pos_valid(neighbour_pos)) {
                            bit++;
                            continue;
                        }
                        assets::Map::Tile neighbour = layer.get_tile(neighbour_pos);
                        bool is_neighbour_of_same_type =
                            tileset.get_x_index_from_auto_id(neighbour.id) ==
                            tileset.get_x_index_from_auto_id(self_tile.id);
                        surroundings ^= is_neighbour_of_same_type << bit;
                        bit++;
                    }
                }
                layer.set_tile(pos,
                               {tileset.get_id_auto(tileset.get_x_index_from_auto_id(self_tile.id),
                                                    surroundings)});
            };
            // Set the tile below the cursor and don't worry about the surroundings; we'll update
            // them later
            layer.set_tile(pos, {tileset.get_id_auto(selection.selection_start.x, 0)});
            // Update autoID of tile placed and all others near it
            for (int iy = -1; iy <= 1; ++iy) {
                for (int ix = -1; ix <= 1; ++ix) {
                    const math::IVec2D ipos = {pos.x + ix, pos.y + iy};
                    if (!layer.is_pos_valid(ipos))
                        continue;
                    update_auto_id(ipos);
                }
            }
        } break;

        default: ARPIYI_UNREACHABLE(); break;
    }
}

static void draw_selection_on_map(assets::Map& map,
                                  bool is_tileset_appropiate_for_layer,
                                  ImVec2 map_render_pos,
                                  ImVec2 relative_mouse_pos,
                                  math::IVec2D mouse_tile_pos,
                                  ImVec2 content_start_pos) {
    auto selection = tileset_manager::get_selection();
    if (auto selection_tileset = selection.tileset.get()) {
        ImVec2 selection_render_pos =
            ImVec2(relative_mouse_pos.x + map_render_pos.x + content_start_pos.x,
                   relative_mouse_pos.y + map_render_pos.y + content_start_pos.y);
        ImVec2 map_selection_size =
            ImVec2{(float)(selection.selection_end.x + 1 - selection.selection_start.x) *
                       tileset_manager::get_tile_size() * get_map_zoom(),
                   (float)(selection.selection_end.y + 1 - selection.selection_start.y) *
                       tileset_manager::get_tile_size() * get_map_zoom()};
        math::IVec2D tileset_size = selection_tileset->get_size_in_tiles();
        ImVec2 uv_min = ImVec2{(float)selection.selection_start.x / (float)tileset_size.x,
                               (float)selection.selection_start.y / (float)tileset_size.y};
        ImVec2 uv_max = ImVec2{(float)(selection.selection_end.x + 1) / (float)tileset_size.x,
                               (float)(selection.selection_end.y + 1) / (float)tileset_size.y};
        ImVec2 clip_rect_min = {map_render_pos.x + content_start_pos.x,
                                map_render_pos.y + content_start_pos.y};

        ImGui::GetWindowDrawList()->PushClipRect(
            clip_rect_min,
            {clip_rect_min.x + map.width * tileset_manager::get_tile_size() * get_map_zoom(),
             clip_rect_min.y + map.height * tileset_manager::get_tile_size() * get_map_zoom()},
            true);
        ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<ImTextureID>(selection_tileset->texture.get()->handle),
            selection_render_pos,
            {selection_render_pos.x + map_selection_size.x,
             selection_render_pos.y + map_selection_size.y},
            uv_min, uv_max, ImGui::GetColorU32({1.f, 1.f, 1.f, 0.4f}));
        ImGui::GetWindowDrawList()->PopClipRect();

        ImGui::SetCursorScreenPos(selection_render_pos);
        ImGui::InvisibleButton("##_map_img", {map_selection_size.x, map_selection_size.y});
        if (is_tileset_appropiate_for_layer) {
            if (ImGui::IsItemHovered() && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                if (current_layer_selected.get()) {
                    place_tile_on_pos(map, mouse_tile_pos);
                }
            }
        }
    }
}

void init() {
    tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, (float)0.0f);
}

Handle<assets::Map> get_current_map() { return current_map; }

void render() {
    auto map = current_map.get();

    if (ImGui::Begin(ICON_MD_TERRAIN " Map View", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
                         ImGuiWindowFlags_NoScrollWithMouse)) {
        if (map) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Checkbox("Grid", &show_grid);

                ImGui::EndMenuBar();
            }

            if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered()) {
                if (ImGui::GetIO().MouseWheel > 0 && current_zoom_level < zoom_levels.size() - 1)
                    current_zoom_level += 1;
                else if (ImGui::GetIO().MouseWheel < 0 && current_zoom_level > 0)
                    current_zoom_level -= 1;
            }

            math::IVec2D map_render_pos{
                static_cast<int>(map_scroll.x * get_map_zoom() + ImGui::GetWindowWidth() / 2.f),
                static_cast<int>(map_scroll.y * get_map_zoom() + ImGui::GetWindowHeight() / 2.f)};

            ImVec2 abs_content_start_pos = {
                ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x,
                ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y};
            {
                // Draw the map
                static DrawMapCallbackData data;
                data = {ImVec2{static_cast<float>(map_render_pos.x),
                               static_cast<float>(map_render_pos.y)},
                        abs_content_start_pos};
                ImGui::GetWindowDrawList()->AddCallback(&draw_map_callback,
                                                        static_cast<void*>(&data));
                ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
            }

            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 relative_mouse_pos =
                ImVec2(mouse_pos.x - (map_render_pos.x + abs_content_start_pos.x),
                       mouse_pos.y - (map_render_pos.y + abs_content_start_pos.y));

            // Snap the relative mouse position
            ImVec2 snapped_relative_mouse_pos{
                static_cast<float>(static_cast<int>(relative_mouse_pos.x) -
                                   static_cast<int>(std::fmod(
                                       relative_mouse_pos.x,
                                       (tileset_manager::get_tile_size() * get_map_zoom())))),

                static_cast<float>(static_cast<int>(relative_mouse_pos.y) -
                                   static_cast<int>(std::fmod(
                                       relative_mouse_pos.y,
                                       (tileset_manager::get_tile_size() * get_map_zoom()))))};
            math::IVec2D mouse_tile_pos = {
                static_cast<i32>(snapped_relative_mouse_pos.x /
                                 (tileset_manager::get_tile_size() * get_map_zoom())),
                static_cast<i32>(snapped_relative_mouse_pos.y /
                                 (tileset_manager::get_tile_size() * get_map_zoom()))};

            bool is_tileset_appropiate_for_layer;
            if (auto layer = current_layer_selected.get())
                is_tileset_appropiate_for_layer =
                    tileset_manager::get_selection().tileset == layer->tileset;
            else
                is_tileset_appropiate_for_layer = true;

            draw_selection_on_map(
                *map, is_tileset_appropiate_for_layer,
                {static_cast<float>(map_render_pos.x), static_cast<float>(map_render_pos.y)},
                snapped_relative_mouse_pos, mouse_tile_pos, abs_content_start_pos);

            static bool show_text_comment_creation_window = false;
            static math::IVec2D comment_creation_pos;
            if (is_tileset_appropiate_for_layer) {
                if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseDown[ImGuiMouseButton_Middle]) {
                    ImGui::SetWindowFocus();
                    ImGuiIO& io = ImGui::GetIO();
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    map_scroll = ImVec2{map_scroll.x + io.MouseDelta.x / get_map_zoom(),
                                        map_scroll.y + io.MouseDelta.y / get_map_zoom()};
                }
                if (ImGui::BeginPopupContextWindow("##_map_ctx")) {
                    if (ImGui::IsWindowAppearing())
                        comment_creation_pos = mouse_tile_pos;

                    if (ImGui::Selectable("Create text comment")) {
                        show_text_comment_creation_window = true;
                    }
                    ImGui::EndPopup();
                }
            }

            if (show_text_comment_creation_window) {
                static char notes[1024];
                ImGui::SetNextWindowSize({400, 200}, ImGuiCond_Once);
                ImGui::Begin(ICON_MD_INSERT_COMMENT " Insert Text Comment",
                             &show_text_comment_creation_window);
                ImGui::InputTextMultiline("", notes, 1024);
                if (ImGui::Button("OK")) {
                    map->comments.emplace_back(
                        assets::Map::Comment{std::string(notes), comment_creation_pos});
                    show_text_comment_creation_window = false;
                }
                ImGui::End();
            }

            for (auto& comment : map->comments) {
                ImVec2 comment_square_render_pos_min = {
                    comment.pos.x * tileset_manager::get_tile_size() * get_map_zoom() +
                        map_render_pos.x + abs_content_start_pos.x,
                    comment.pos.y * tileset_manager::get_tile_size() * get_map_zoom() +
                        map_render_pos.y + abs_content_start_pos.y};
                ImVec2 comment_square_render_pos_max = {
                    comment_square_render_pos_min.x +
                        tileset_manager::get_tile_size() * get_map_zoom(),
                    comment_square_render_pos_min.y +
                        tileset_manager::get_tile_size() * get_map_zoom()};
                ImGui::GetWindowDrawList()->AddRect(
                    comment_square_render_pos_min, comment_square_render_pos_max,
                    ImGui::GetColorU32({0.9f, 0.8f, 0.05f, 0.6f}), 0, ImDrawCornerFlags_All, 5.f);
                if (ImGui::IsMouseHoveringRect(comment_square_render_pos_min,
                                               comment_square_render_pos_max)) {
                    ImGui::BeginTooltip();
                    ImGui::TextDisabled("Text comment at {%i, %i}", comment.pos.x, comment.pos.y);
                    ImGui::Separator();
                    ImGui::TextUnformatted(comment.text.c_str());
                    ImGui::EndTooltip();
                }
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

            draw_pos_info_bar(mouse_tile_pos, relative_mouse_pos);
        } else
            ImGui::TextDisabled("No map loaded to view.");
    }
    ImGui::End();

    static bool show_add_layer = false;
    if (ImGui::Begin(ICON_MD_LAYERS " Map Layers", nullptr, ImGuiWindowFlags_MenuBar)) {
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
                    if (l == current_layer_selected ? ImGui::TextUnformatted(layer.name.c_str())
                                                    : ImGui::TextDisabled("%s", layer.name.c_str()),
                        ImGui::IsItemClicked()) {
                        current_layer_selected = l;
                        tileset_manager::set_selection_tileset(layer.tileset);
                    }

                    if (ImGui::BeginPopupContextItem("Layer Context Menu")) {
                        if (ImGui::Selectable("Delete"))
                            layer_to_delete = l;
                        ImGui::EndPopup();
                    }
                }

                if (layer_to_delete.get_id() != Handle<assets::Map::Layer>::noid) {
                    layer_to_delete.unload();
                    map->layers.erase(
                        std::remove(map->layers.begin(), map->layers.end(), layer_to_delete),
                        map->layers.end());
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

    static bool show_add_map = false;
    if (ImGui::Begin(ICON_MD_VIEW_LIST " Map List", nullptr, ImGuiWindowFlags_MenuBar)) {
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
                    current_map = Handle<assets::Map>(_id);
                    if (!current_map.get()->layers.empty()) {
                        current_layer_selected = current_map.get()->layers[0];
                        tileset_manager::set_selection_tileset(
                            current_layer_selected.get()->tileset);
                    }
                }
            }
    }
    ImGui::End();

    if (show_add_map) {
        show_add_map_window(&show_add_map);
    }
}

std::vector<Handle<assets::Map>> get_maps() {
    std::vector<Handle<assets::Map>> maps;
    for (const auto& [id, map] : detail::AssetContainer<assets::Map>::get_instance().map) {
        maps.emplace_back(id);
    }
    return maps;
}

} // namespace arpiyi_editor::map_manager