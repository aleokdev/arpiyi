#include "startup_dialog.hpp"
#include "serializing_manager.hpp"

#include "util/icons_material_design.hpp"
#include "project_info.hpp"

#include <imgui.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <noc_file_dialog.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

namespace fs = std::filesystem;

namespace arpiyi_editor::startup_dialog {

struct RecentProjectData {
    fs::path path;
};
static std::vector<RecentProjectData> recent_projects;
static constexpr std::string_view arpiyi_config_file_path = "config.json";

namespace config_file_definitions {
constexpr std::string_view recent_projects_json_key = "recent_projects";
}

static void create_config_file() {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace config_file_definitions;

    w.StartObject();

    w.Key(recent_projects_json_key.data());
    w.StartArray();
    w.EndArray();
    w.EndObject();

    {
        auto config_file = std::ofstream(fs::path(arpiyi_config_file_path));
        config_file << s.GetString();
    }
}

static void add_path_to_config_file_recent_projects(fs::path const& path) {
    rapidjson::Document doc;
    {
        auto f = std::ifstream(fs::path(arpiyi_config_file_path));
        std::stringstream buffer;
        buffer << f.rdbuf();

        doc.Parse(buffer.str().data());
    }

    using namespace config_file_definitions;
    std::string path_str = path.generic_string();
    rapidjson::Value val;
    val.SetString(path_str.data(), path_str.size());
    doc.GetObject()[recent_projects_json_key.data()].GetArray().PushBack(val, doc.GetAllocator());

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    doc.Accept(writer);

    auto f = std::ofstream(fs::path(arpiyi_config_file_path));
    f << s.GetString();
}

void init() {
    if(!fs::exists(arpiyi_config_file_path)) {
        create_config_file();
        return;
    }

    auto f = std::ifstream(fs::path(arpiyi_config_file_path));
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    using namespace config_file_definitions;
    for(auto& recent_project_path : doc.GetObject()[recent_projects_json_key.data()].GetArray()) {
        recent_projects.emplace_back(RecentProjectData {std::string(recent_project_path.GetString())});
    }
}

void render(bool* show_demo_window) {
    ImGui::SetNextWindowSize({500, 200}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Startup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted("Welcome to Arpiyi Editor v." ARPIYI_EDITOR_VERSION);
        ImGui::Separator();
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
        ImGui::BeginChild("recent_projects", {150,ImGui::GetContentRegionAvail().y}, true);
        ImGui::TextDisabled("Recent Projects");
        for(auto& recent_project : recent_projects) {
            if(ImGui::Button(recent_project.path.filename().generic_string().c_str())) {
                if (!fs::is_directory(recent_project.path)) {
                    recent_project.path = recent_project.path.parent_path();
                }

                serializing_manager::start_load(recent_project.path);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
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
                add_path_to_config_file_recent_projects(base_dir);

                serializing_manager::start_load(base_dir);
                ImGui::CloseCurrentPopup();
            }
        }
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Experimental, extremely buggy");
            ImGui::EndTooltip();
        }
        if (ImGui::Button(ICON_MD_WIDGETS " Open ImGui Demo")) {
            *show_demo_window = true;
        }
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("You can also press Ctrl+I later on to open it");
            ImGui::EndTooltip();
        }
        ImGui::EndGroup();
        ImGui::EndPopup();
    }
    static bool first_time = true;
    if (first_time) {
        ImGui::OpenPopup("Startup");
        first_time = false;
    }
}

} // namespace arpiyi_editor::startup_dialog