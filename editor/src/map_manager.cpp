#include "map_manager.hpp"

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

std::vector<Handle<assets::Map>> maps;
Handle<assets::Map> current_map;
Handle<assets::Shader> tile_shader;
Handle<assets::Shader> grid_shader;
Handle<assets::Mesh> quad_mesh;
unsigned int map_view_framebuffer;
assets::Texture grid_view_texture;
glm::mat4 proj_mat;
std::size_t current_layer_selected = 0;
ImVec2 map_scroll{0, 0};

/// Updates grid_view_texture to fit the width and height of current_map.
// TODO: Replace with imgui custom callbacks, remove framebuffer
static void update_grid_view_texture() {
    auto map = current_map.get();
    if (!map)
        return;

    if (grid_view_texture.handle == assets::Texture::nohandle)
        glDeleteTextures(1, &grid_view_texture.handle);
    glGenTextures(1, &grid_view_texture.handle);
    glBindTexture(GL_TEXTURE_2D, grid_view_texture.handle);

    grid_view_texture.w = map->width * tileset_manager::get_tile_size();
    grid_view_texture.h = map->height * tileset_manager::get_tile_size();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, grid_view_texture.w, grid_view_texture.h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           grid_view_texture.handle, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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
            current_map.get()->layers.emplace_back(layer);
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
    if (ImGui::Begin(ICON_MD_ADD_BOX " New Map", p_open)) {
        static char name[32] = "Default";
        static i32 map_size[2] = {16, 16};
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

        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("OK")) {
            assets::Map map;
            map.width = static_cast<decltype(map.width)>(map_size[0]);
            map.height = static_cast<decltype(map.height)>(map_size[1]);
            map.name = name;
            current_map = maps.emplace_back(asset_manager::put<assets::Map>(map));
            update_grid_view_texture();
            *p_open = false;
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
        char buf[128];
        sprintf(buf, "Tile pos: {%i, %i} - Mouse pos: {%.0f, %.0f}", tile_pos.x, tile_pos.y,
                relative_mouse_pos.x, relative_mouse_pos.y);
        ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), buf);
    }
}

static void draw_map_to_fb(assets::Map const& map, bool show_grid) {
    glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!map.layers.empty()) {
        glUseProgram(tile_shader.get()->handle);
        glActiveTexture(GL_TEXTURE0);
        // Draw each layer
        for (auto& layer : current_map.get()->layers) {
            if (!layer.visible)
                continue;

            glBindVertexArray(layer.get_mesh().get()->vao);
            glBindTexture(GL_TEXTURE_2D, layer.tileset.get()->texture.get()->handle);
            glViewport(0.f, 0.f, grid_view_texture.w, grid_view_texture.h);
            glm::mat4 model = glm::mat4(1);
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
        glUniform4f(3, .9f, .9f, .9f, .4f);
        glUniform2ui(4, current_map.get()->width, current_map.get()->height);
        glViewport(0.f, 0.f, grid_view_texture.w, grid_view_texture.h);
        glm::mat4 model = glm::mat4(1);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
                    map.layers[current_layer_selected].set_tile(
                        pos, {map.layers[current_layer_selected].tileset.get()->get_id({tx, ty})});
                    pos.y++;
                }
                pos.x++;
                pos.y = start_my;
            }
        } break;

        case (assets::Tileset::AutoType::rpgmaker_a2): {
            auto& layer = map.layers[current_layer_selected];
            const auto& tileset = *selection.tileset.get();
            const assets::Map::Tile self_tile = layer.get_tile(pos);
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

void init() {
    glGenFramebuffers(1, &map_view_framebuffer);

    tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, (float)0.0f);
    update_grid_view_texture();
}

Handle<assets::Map> get_current_map() { return current_map; }

void render() {
    static bool show_grid = true;
    auto map = current_map.get();
    if (map)
        draw_map_to_fb(*map, show_grid);

    if (ImGui::Begin(ICON_MD_TERRAIN " Map View", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
        if (map) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Checkbox("Grid", &show_grid);

                ImGui::EndMenuBar();
            }

            ImVec2 base_cursor_pos{ImGui::GetCursorScreenPos().x + map_scroll.x,
                                   ImGui::GetCursorScreenPos().y + map_scroll.y};
            ImGui::SetCursorScreenPos(base_cursor_pos);
            ImGui::Image(reinterpret_cast<ImTextureID>(grid_view_texture.handle),
                         ImVec2{(float)grid_view_texture.w, (float)grid_view_texture.h});

            ImVec2 mouse_pos = ImGui::GetIO().MousePos;
            ImVec2 relative_mouse_pos =
                ImVec2(mouse_pos.x - base_cursor_pos.x, mouse_pos.y - base_cursor_pos.y);

            // Snap the relative mouse position
            relative_mouse_pos.x = static_cast<int>(relative_mouse_pos.x) -
                                   static_cast<int>(static_cast<int>(relative_mouse_pos.x) %
                                                    tileset_manager::get_tile_size());
            relative_mouse_pos.y = static_cast<int>(relative_mouse_pos.y) -
                                   static_cast<int>(static_cast<int>(relative_mouse_pos.y) %
                                                    tileset_manager::get_tile_size());
            math::IVec2D mouse_tile_pos = {
                static_cast<i32>(relative_mouse_pos.x / tileset_manager::get_tile_size()),
                static_cast<i32>(relative_mouse_pos.y / tileset_manager::get_tile_size())};

            auto selection = tileset_manager::get_selection();
            if (auto selection_tileset = selection.tileset.get()) {
                ImGui::SetCursorScreenPos(base_cursor_pos);

                ImVec2 selection_render_pos = ImVec2(relative_mouse_pos.x + base_cursor_pos.x,
                                                     relative_mouse_pos.y + base_cursor_pos.y);
                ImVec2 selection_size =
                    ImVec2{(float)(selection.selection_end.x + 1 - selection.selection_start.x) *
                               tileset_manager::get_tile_size(),
                           (float)(selection.selection_end.y + 1 - selection.selection_start.y) *
                               tileset_manager::get_tile_size()};
                math::IVec2D tileset_size = selection_tileset->get_size_in_tiles();
                ImVec2 uv_min =
                    ImVec2{(float)selection.selection_start.x * 1.f / (float)tileset_size.x,
                           (float)selection.selection_start.y * 1.f / (float)tileset_size.y};
                ImVec2 uv_max =
                    ImVec2{(float)(selection.selection_end.x + 1) * 1.f / (float)tileset_size.x,
                           (float)(selection.selection_end.y + 1) * 1.f / (float)tileset_size.y};

                ImGui::GetWindowDrawList()->PushClipRect(
                    base_cursor_pos,
                    {base_cursor_pos.x + map->width * tileset_manager::get_tile_size(),
                     base_cursor_pos.y + map->height * tileset_manager::get_tile_size()},
                    true);
                ImGui::GetWindowDrawList()->AddImage(
                    reinterpret_cast<ImTextureID>(selection_tileset->texture.get()->handle),
                    selection_render_pos,
                    {selection_render_pos.x + selection_size.x,
                     selection_render_pos.y + selection_size.y},
                    uv_min, uv_max, ImGui::GetColorU32({1, 1, 1, 0.4f}));
                ImGui::GetWindowDrawList()->PopClipRect();
            }
            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) ||
                ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow)) {
                if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                    if (!map->layers.empty()) {
                        place_tile_on_pos(*map, mouse_tile_pos);
                    }
                }

                if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Middle]) {
                    ImGui::SetWindowFocus();
                    ImGuiIO& io = ImGui::GetIO();
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                    map_scroll =
                        ImVec2{map_scroll.x + io.MouseDelta.x, map_scroll.y + io.MouseDelta.y};
                }
            }

            ImVec2 rel_mouse_px_pos =
                ImVec2(mouse_pos.x - base_cursor_pos.x, mouse_pos.y - base_cursor_pos.y);
            draw_pos_info_bar({mouse_tile_pos.x, mouse_tile_pos.y}, rel_mouse_px_pos);
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
                std::size_t i_to_delete = -1;
                std::size_t i = 0;
                for (auto& layer : map->layers) {
                    ImGui::TextDisabled("%zu", i);
                    ImGui::SameLine();
                    if (ImGui::TextDisabled("%s", layer.visible ? ICON_MD_VISIBILITY
                                                                : ICON_MD_VISIBILITY_OFF),
                        ImGui::IsItemClicked()) {
                        layer.visible = !layer.visible;
                    }
                    ImGui::SameLine();
                    if (i == current_layer_selected ? ImGui::TextUnformatted(layer.name.c_str())
                                                    : ImGui::TextDisabled("%s", layer.name.c_str()),
                        ImGui::IsItemClicked()) {
                        current_layer_selected = i;
                    }

                    if (ImGui::BeginPopupContextItem("Layer Context Menu")) {
                        if (ImGui::Selectable("Delete"))
                            i_to_delete = i;
                        ImGui::EndPopup();
                    }
                    i++;
                }

                if (i_to_delete != static_cast<std::size_t>(-1)) {
                    map->layers.erase(map->layers.begin() + i_to_delete);
                    current_layer_selected--;
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
        if (maps.empty())
            ImGui::TextDisabled("No maps");
        else
            for (auto& _m : maps) {
                if (auto i_map = _m.get()) {
                    ImGui::TextDisabled("%zu", _m.get_id());
                    ImGui::SameLine();
                    if (ImGui::Selectable(i_map->name.c_str(), _m == current_map)) {
                        current_map = _m;
                        update_grid_view_texture();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, 0.f, 0.f, 1.f});
                    ImGui::TextUnformatted("Empty reference");
                    ImGui::PopStyleColor(1);
                }
            }
    }
    ImGui::End();

    if (show_add_map) {
        show_add_map_window(&show_add_map);
    }
}

std::vector<Handle<assets::Map>>& get_maps() { return maps; }

} // namespace arpiyi_editor::map_manager