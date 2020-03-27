#include "map_manager.hpp"

#include <iostream>
#include <vector>

#include "assets/shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <window_manager.hpp>

#include "tileset_manager.hpp"

namespace arpiyi_editor::map_manager {

std::vector<asset_manager::Handle<assets::Map>> maps;
asset_manager::Handle<assets::Map> current_map;
asset_manager::Handle<assets::Mesh> current_map_display_mesh;
asset_manager::Handle<assets::Shader> tile_shader;
asset_manager::Handle<assets::Shader> grid_shader;
asset_manager::Handle<assets::Mesh> quad_mesh;
unsigned int map_view_framebuffer;
assets::Texture map_view_texture;
glm::mat4 proj_mat;

static void update_view_texture() {
    auto map = current_map.get();
    if(!map) return;

    if (map_view_texture.handle == -1)
        glDeleteTextures(1, &map_view_texture.handle);
    glGenTextures(1, &map_view_texture.handle);
    glBindTexture(GL_TEXTURE_2D, map_view_texture.handle);

    map_view_texture.w = map->width * tileset_manager::get_tile_size();
    map_view_texture.h = map->height * tileset_manager::get_tile_size();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, map_view_texture.w, map_view_texture.h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering (Because it needs mipmaps, which we haven't set)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           map_view_texture.handle, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void update_current_map_display_mesh() {
    auto mesh = current_map_display_mesh.get();
    auto map = current_map.get();
    if(!map) return;
    if(mesh) {
        mesh->destroy();
        (*mesh) = assets::Mesh::generate_split_quad(map->width, map->height);
    } else {
        current_map_display_mesh = asset_manager::put(assets::Mesh::generate_split_quad(map->width, map->height));
    }
}

void init() {
    glGenFramebuffers(1, &map_view_framebuffer);

    tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    grid_shader = asset_manager::load<assets::Shader>({"data/grid.vert", "data/grid.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_quad());

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, (float)0.0f);
    update_view_texture();
    update_current_map_display_mesh();
}

asset_manager::Handle<assets::Map> get_current_map() { return current_map; }

void render() {
    auto mesh = current_map_display_mesh.get();
    if(mesh) {
        glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
        if(!current_map.get()->layers.empty()) {
            glUseProgram(tile_shader.get()->handle);
            glBindVertexArray(current_map_display_mesh.get()->vao);
            glActiveTexture(GL_TEXTURE0);
            // Draw each layer
            for (auto& layer : current_map.get()->layers) {
                glBindTexture(GL_TEXTURE_2D, layer.tileset.get()->texture.get()->handle);
                glViewport(0.f, 0.f, map_view_texture.w, map_view_texture.h);
                glm::mat4 model = glm::mat4(1);
                glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

                constexpr int quad_verts = 2 * 3;
                glDrawArrays(GL_TRIANGLES, 0, 4 * 8 * quad_verts);
            }
        }
        // Draw mesh grid
        glUseProgram(grid_shader.get()->handle);
        glBindVertexArray(quad_mesh.get()->vao);
        glUniform4f(3, .9f, .9f, .9f, .4f);
        glUniform2ui(4, current_map.get()->width, current_map.get()->height);
        glViewport(0.f, 0.f, map_view_texture.w, map_view_texture.h);
        glm::mat4 model = glm::mat4(1);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

        constexpr int quad_verts = 2 * 3;
        glDrawArrays(GL_TRIANGLES, 0, quad_verts);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (!ImGui::Begin("Map View")) {
        ImGui::End();
    } else {
        if(mesh)
            ImGui::Image(reinterpret_cast<ImTextureID>(map_view_texture.handle),
                         ImVec2{(float)map_view_texture.w, (float)map_view_texture.h});
        else
            ImGui::TextDisabled("No map loaded to view.");
        ImGui::End();
    }

    if (!ImGui::Begin("Map Layers", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
    } else {
        if (auto map = current_map.get()) {
            if (map->layers.empty())
                ImGui::TextDisabled("No layers in current map");
            else {
                std::size_t i = 0;
                for (auto& layer : map->layers) {
                    ImGui::TextDisabled("%zu", i);
                    ImGui::SameLine();
                    ImGui::TextUnformatted(layer.name.c_str());
                }
            }
        } else {
            ImGui::TextDisabled("No map selected");
        }

        ImGui::End();
    }

    static bool show_add_map = false;
    if (!ImGui::Begin("Map List", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
    } else {
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
                        update_current_map_display_mesh();
                        update_view_texture();
                    }
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{.8f, 0.f, 0.f, 1.f});
                    ImGui::TextUnformatted("Empty reference");
                    ImGui::PopStyleColor(1);
                }
            }
        ImGui::End();
    }

    if (show_add_map) {
        if (!ImGui::Begin("New Map", &show_add_map)) {
            ImGui::End();
        } else {
            static char name[32];
            static int map_size[2];
            asset_manager::Handle<assets::Tileset> tileset;
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
            /*
                        auto t = tileset.get();
                        if(ImGui::BeginCombo("Tileset", t ? t->name.c_str() : "<Not selected>")) {
                            for(auto& ts : tileset_manager::get_tilesets()) {
                                if(ImGui::Selectable(ts.get()->name.c_str(), ts == tileset))
                                    tileset = ts;

                                if(ts == tileset)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }*/

            if (ImGui::Button("Cancel")) {
                show_add_map = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("OK")) {
                assets::Map start_map;
                start_map.width = map_size[0];
                start_map.height = map_size[1];
                start_map.name = name;
                current_map = maps.emplace_back(asset_manager::put<assets::Map>(start_map));
                update_current_map_display_mesh();
                update_view_texture();
                show_add_map = false;
            }
            ImGui::End();
        }
    }
}

} // namespace arpiyi_editor::map_manager