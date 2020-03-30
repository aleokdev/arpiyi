#include "map_manager.hpp"

#include <iostream>
#include <vector>
#include <cmath>

#include "assets/shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <window_manager.hpp>

#include "tileset_manager.hpp"

namespace arpiyi_editor::map_manager {

std::vector<asset_manager::Handle<assets::Map>> maps;
asset_manager::Handle<assets::Map> current_map;
asset_manager::Handle<assets::Shader> tile_shader;
asset_manager::Handle<assets::Shader> grid_shader;
asset_manager::Handle<assets::Mesh> quad_mesh;
unsigned int map_view_framebuffer;
assets::Texture grid_view_texture;
glm::mat4 proj_mat;
std::size_t current_layer_selected = 0;

/// Updates grid_view_texture to fit the width and height of current_map.
// TODO: Replace with imgui custom callbacks, remove framebuffer
static void update_grid_view_texture() {
    auto map = current_map.get();
    if (!map)
        return;

    if (grid_view_texture.handle == -1)
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
    if (!ImGui::Begin("New Map Layer", p_open)) {
        ImGui::End();
    } else {
        static char name[32];
        static asset_manager::Handle<assets::Tileset> tileset;
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
        ImGui::End();
    }
}

static void show_add_map_window(bool* p_open) {
    if (!ImGui::Begin("New Map", p_open)) {
        ImGui::End();
    } else {
        static char name[32];
        static int map_size[2];
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
            map.width = map_size[0];
            map.height = map_size[1];
            map.name = name;
            current_map = maps.emplace_back(asset_manager::put<assets::Map>(map));
            update_grid_view_texture();
            *p_open = false;
        }
        ImGui::End();
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

asset_manager::Handle<assets::Map> get_current_map() { return current_map; }

void render() {
    auto map = current_map.get();
    if (map) {
        glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!map->layers.empty()) {
            glUseProgram(tile_shader.get()->handle);
            glActiveTexture(GL_TEXTURE0);
            // Draw each layer
            for (auto& layer : current_map.get()->layers) {
                glBindVertexArray(layer.get_mesh().const_get()->vao);
                glBindTexture(GL_TEXTURE_2D, layer.tileset.get()->texture.get()->handle);
                glViewport(0.f, 0.f, grid_view_texture.w, grid_view_texture.h);
                glm::mat4 model = glm::mat4(1);
                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0, map->width * map->height * quad_verts);
            }
        }
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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (ImGui::Begin("Map View")) {
        if (map) {
            ImVec2 base_cursor_pos = ImGui::GetCursorScreenPos();
            ImGui::Image(reinterpret_cast<ImTextureID>(grid_view_texture.handle),
                         ImVec2{(float)grid_view_texture.w, (float)grid_view_texture.h});

            auto selection = tileset_manager::get_selection();
            if (auto selection_tileset = selection.tileset.get()) {
                ImGui::SetCursorScreenPos(base_cursor_pos);

                ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                ImVec2 relative_mouse_pos =
                    ImVec2(mouse_pos.x - base_cursor_pos.x, mouse_pos.y - base_cursor_pos.y);

                // Snap the relative mouse position
                relative_mouse_pos.x =
                    (int)relative_mouse_pos.x -
                    ((int)relative_mouse_pos.x % tileset_manager::get_tile_size());
                relative_mouse_pos.y =
                    (int)relative_mouse_pos.y -
                    ((int)relative_mouse_pos.y % tileset_manager::get_tile_size());

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

                if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                    int mx = static_cast<int>(relative_mouse_pos.x /
                                              tileset_manager::get_tile_size()),
                        my = static_cast<int>(relative_mouse_pos.y /
                                              tileset_manager::get_tile_size());
                    int start_my = my;
                    if(mx >= 0 && my >= 0 && mx < map->width && my < map->height && !map->layers.empty()) {
                        for (int tx = selection.selection_start.x; tx <= selection.selection_end.x;
                             tx++) {
                            for (int ty = selection.selection_start.y;
                                 ty <= selection.selection_end.y; ty++) {
                                if(!(mx >= 0 && my >= 0 && mx < map->width && my < map->height)) continue;
                                map->layers[current_layer_selected].set_tile(
                                    {mx, my}, {map->layers[current_layer_selected].tileset.get()->get_id({tx, ty})});
                                my++;
                            }
                            mx++;
                            my = start_my;
                        }
                    }
                }
            }
        } else
            ImGui::TextDisabled("No map loaded to view.");
    }
    ImGui::End();

    static bool show_add_layer = false;
    if (ImGui::Begin("Map Layers", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New...")) {
                show_add_layer = true;
            }
            ImGui::EndMenuBar();
        }

        if (auto map = current_map.get()) {
            if (map->layers.empty())
                ImGui::TextDisabled("No layers in current map");
            else {
                std::size_t i = 0;
                for (auto& layer : map->layers) {
                    ImGui::TextDisabled("%zu", i);
                    ImGui::SameLine();
                    if (ImGui::Selectable(layer.name.c_str(), i == current_layer_selected)) {
                        current_layer_selected = i;
                    }
                    i++;
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
    if (ImGui::Begin("Map List", nullptr, ImGuiWindowFlags_MenuBar)) {
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
                if (auto map = _m.get()) {
                    ImGui::TextDisabled("%zu", _m.get_id());
                    ImGui::SameLine();
                    if (ImGui::Selectable(map->name.c_str(), _m == current_map)) {
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

} // namespace arpiyi_editor::map_manager