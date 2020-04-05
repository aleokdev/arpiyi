#include "startup_dialog.hpp"
#include <imgui.h>

#include "serializer.hpp"

#include "util/icons_material_design.hpp"
#include "project_info.hpp"

#include <noc_file_dialog.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi_editor::startup_dialog {

void render() {
    static bool show_imgui_demo = false;
    ImGui::SetNextWindowSize({500, 200}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Startup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("Welcome to Arpiyi Editor v." ARPIYI_EDITOR_VERSION);
        ImGui::Separator();
        ImGui::BeginChild("recent_projects", {150,ImGui::GetContentRegionAvail().y}, true);
        if(ImGui::Button("Recent Project 1")) {}
        if(ImGui::Button("Recent Project 2")) {}
        if(ImGui::Button("Recent Project 3")) {}
        if(ImGui::Button("Recent Project 4")) {}
        if(ImGui::Button("Recent Project 5")) {}
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        if (ImGui::Button(ICON_MD_ADD " New Project")) {
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(ICON_MD_FOLDER " Load Project")) {
            if (const char* c_path =
                    noc_file_dialog_open(NOC_FILE_DIALOG_DIR, nullptr, nullptr, nullptr)) {
                fs::path base_dir(c_path);
                if (!fs::is_directory(base_dir)) {
                    base_dir = base_dir.parent_path();
                }

                serializer::load_project(base_dir);
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Experimental, extremely buggy");
            ImGui::EndTooltip();
        }
        if (ImGui::Button(ICON_MD_WIDGETS " Open ImGui Demo")) {
            show_imgui_demo = true;
        }
        ImGui::EndGroup();
        ImGui::EndPopup();
    }
    static bool first_time = true;
    if (first_time) {
        ImGui::OpenPopup("Startup");
        first_time = false;
    }
    if (show_imgui_demo) {
        ImGui::ShowDemoWindow(&show_imgui_demo);
    }
}

} // namespace arpiyi_editor::startup_dialog