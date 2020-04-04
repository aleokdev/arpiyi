#include "editor/editor_renderer.hpp"
#include <imgui.h>

#include <iostream>
#include <map>
#include <noc_file_dialog.h>
#include <serializer.hpp>
#include <unordered_map>
#include <unordered_set>

namespace arpiyi_editor::editor::renderer {

std::map<std::size_t, Toolbar> toolbars;
std::unordered_set<std::size_t>
    toolbars_to_remove; // Needed because lua might delete a toolbar while rendering (Which will
                        // cause issues with the for loop)
std::size_t last_toolbar_id = 0;

Toolbar& add_toolbar(std::string const& name, std::function<void(Toolbar&)> const& callback) {
    const auto id_to_use = last_toolbar_id++;
    return toolbars.emplace(id_to_use, Toolbar{id_to_use, name, callback}).first->second;
}

void remove_toolbar(std::size_t id) { toolbars_to_remove.emplace(id); }

bool has_toolbar(std::size_t id) {
    return toolbars.find(id) != toolbars.end() &&
           std::find(toolbars_to_remove.begin(), toolbars_to_remove.end(), id) ==
               toolbars_to_remove.end();
}

std::unordered_map<std::size_t, Window> windows;
std::unordered_set<std::size_t>
    windows_to_remove; // Needed because lua might delete a toolbar while
                       // rendering (Which will cause issues with the for loop)
std::size_t last_window_id = 0;

Window& add_window(std::string const& title) {
    const auto id_to_use = last_window_id++;
    return windows.emplace(id_to_use, Window{id_to_use, title}).first->second;
}

void remove_window(std::size_t id) { windows_to_remove.emplace(id); }

bool has_window(std::size_t id) {
    return windows.find(id) != windows.end() &&
           std::find(windows_to_remove.begin(), windows_to_remove.end(), id) ==
               windows_to_remove.end();
}

void render() {

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground; // Pass-through background

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Main Dock", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("_MainDock");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar()) {
        for (auto& [_, toolbar] : toolbars) {
            if (ImGui::MenuItem(toolbar.name.c_str())) {
                std::cout << "Clicked toolbar!" << std::endl;
                toolbar.callback(toolbar);
            }
        }
        if(ImGui::MenuItem("Save")) {
            if (const char* c_path = noc_file_dialog_open(NOC_FILE_DIALOG_DIR, nullptr, nullptr, nullptr)) {
                if (!fs::is_directory(c_path)) {
                    // TODO: change this to a popup
                    assert(false);
                }
                fs::path base_dir(c_path);

                serializer::save_project(base_dir);
            }
        }
        ImGui::EndMenuBar();
    }

    for (auto it = toolbars.begin(); it != toolbars.end();) {
        if (toolbars_to_remove.find(it->first) != toolbars_to_remove.end())
            it = toolbars.erase(it);
        else
            it++;
    }

    toolbars_to_remove.clear();

    ImGui::End();
}

} // namespace arpiyi_editor::editor::renderer
