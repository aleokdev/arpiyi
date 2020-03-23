#include "map_manager.hpp"

#include <vector>
#include <iostream>

#include "assets/shader.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <window_manager.hpp>

namespace arpiyi_editor::map_manager {

std::vector<asset_manager::Handle<assets::Map>> maps;
asset_manager::Handle<assets::Map> current_map;
asset_manager::Handle<assets::Shader> tile_shader;
asset_manager::Handle<assets::Mesh> quad_mesh;
unsigned int map_view_framebuffer;
assets::Texture map_view_texture;
glm::mat4 proj_mat;

static void update_view_texture() {
    if (map_view_texture.handle == -1)
        glDeleteTextures(1, &map_view_texture.handle);
    glGenTextures(1, &map_view_texture.handle);
    glBindTexture(GL_TEXTURE_2D, map_view_texture.handle);

    map_view_texture.w = current_map.get()->width * 16;
    map_view_texture.h = current_map.get()->height * 16;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, map_view_texture.w, map_view_texture.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // Disable filtering (Because it needs mipmaps)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           map_view_texture.handle, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init() {
    glGenFramebuffers(1, &map_view_framebuffer);

    assets::Map start_map;
    start_map.texture_atlas = asset_manager::load<assets::Texture>({"data/tileset.png"});
    start_map.width = 4;
    start_map.height = 8;
    current_map = maps.emplace_back(asset_manager::put<assets::Map>(start_map));
    current_map.get()->display_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_split_quad(4, 8));
    tile_shader = asset_manager::load<assets::Shader>({"data/tile.vert", "data/tile.frag"});
    quad_mesh = asset_manager::put<assets::Mesh>(assets::Mesh::generate_split_quad(1, 1));

    update_view_texture();
}

asset_manager::Handle<assets::Map> get_current_map() { return current_map; }

void render() {
    glBindFramebuffer(GL_FRAMEBUFFER, map_view_framebuffer);
    glUseProgram(tile_shader.get()->handle);
    glBindVertexArray(current_map.get()->display_mesh.get()->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, current_map.get()->texture_atlas.get()->handle);

    proj_mat = glm::ortho(0.0f, 1.0f, 1.0f, (float)0.0f);
    glViewport(0.f, 0.f, map_view_texture.w, map_view_texture.h);
    glm::mat4 model = glm::mat4(1);

    glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(proj_mat));

    constexpr int quad_verts = 2 * 3;
    glDrawArrays(GL_TRIANGLES, 0, 4 * 8 * quad_verts);
    //glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // why does imgui render this black???
    //glClear(GL_COLOR_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Begin("Map View");
    ImGui::Image((ImTextureID)map_view_texture.handle, ImVec2{(float)map_view_texture.w, (float)map_view_texture.h});
    ImGui::End();
}

} // namespace arpiyi_editor::map_manager