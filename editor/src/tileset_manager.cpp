#include "tileset_manager.hpp"

#include <imgui.h>
#include <noc_file_dialog.h>

namespace arpiyi_editor::tileset_manager {

Tileset current_tileset;

void render() {
    if (!ImGui::Begin("Tileset", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load from image...")) {
                const char* compatible_file_formats =
                    "Any file\0*\0JPEG\0*jpg;*.jpeg\0PNG\0*png\0TGA\0*.tga\0BMP\0*.bmp\0PSD\0"
                    "*.psd\0GIF\0*.gif\0HDR\0*.hdr\0PIC\0*.pic\0PNM\0*.pnm\0";
                std::string_view path_selected = noc_file_dialog_open(
                    NOC_FILE_DIALOG_OPEN, compatible_file_formats, nullptr, nullptr);
                if (path_selected.data() && fs::is_regular_file(path_selected)) {
                    if(current_tileset.texture.get())
                        current_tileset.texture.unload();
                    current_tileset.texture = asset_manager::load<assets::Texture>({path_selected});
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Set scale");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    if (auto img = current_tileset.texture.get())
        ImGui::Image((ImTextureID)img->handle, ImVec2(img->w, img->h));
    else
        ImGui::TextDisabled("No tileset loaded.");

    ImGui::End();
}

} // namespace arpiyi_editor::tileset_manager