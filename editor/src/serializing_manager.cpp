#include "serializing_manager.hpp"

#include "global_tile_size.hpp"
#include "project_info.hpp"
#include "project_manager.hpp"
#include "script_manager.hpp"
#include "serializer.hpp"
#include "serializing_exceptions.hpp"
#include "tileset_manager.hpp"
#include "window_list_menu.hpp"
#include "window_manager.hpp"

#include <imgui.h>
#include <noc_file_dialog.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>

namespace fs = std::filesystem;

namespace arpiyi::serializing_manager {

namespace detail::project_file_definitions {

constexpr std::string_view tile_size_json_key = "tile_size";
constexpr std::string_view editor_version_json_key = "editor_version";
constexpr std::string_view startup_script_id_key = "startup_script_id";

} // namespace detail::project_file_definitions

static void save_project_file(fs::path base_dir) {
    project_manager::set_project_path(base_dir);
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace detail::project_file_definitions;

    w.StartObject();
    {
        w.Key(tile_size_json_key.data());
        w.Uint(global_tile_size::get());
        w.Key(editor_version_json_key.data());
        w.String(ARPIYI_EDITOR_VERSION);
        w.Key(startup_script_id_key.data());
        w.Uint64(script_editor::get_startup_script().get_id());
    }
    w.EndObject();

    {
        std::ofstream project_file(base_dir / "project.json");
        project_file << s.GetString();
    }
}

struct ProjectFileData {
    u32 tile_size = 48;
    std::string editor_version;
};

static ProjectFileData load_project_file(fs::path base_dir) {
    project_manager::set_project_path(base_dir);
    std::ifstream f(base_dir / "project.json");
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    ProjectFileData file_data;

    using namespace detail::project_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == tile_size_json_key.data()) {
            file_data.tile_size = obj.value.GetUint();
        } else if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        } else if (obj.name == startup_script_id_key.data()) {
            script_editor::set_startup_script(obj.value.GetUint64());
        }
    }

    return file_data;
}

float task_progress = 0;
static fs::path project_path;
std::string task_status;
std::function<void()> task_end_callback;

enum class TaskType { saving, loading };
template<TaskType task_type> void task_renderer(bool*) {
    constexpr std::string_view task_type_str = task_type == TaskType::saving ? "Saving" : "Loading";
    static long asset_type_index = -1;

    if (asset_type_index == -1) {
        task_progress = 0.f;
        task_status = std::string(task_type_str) + " project file...";
        if constexpr (task_type == TaskType::saving)
            save_project_file(project_path);
        else
            load_project_file(project_path);
        // Proceed loading assets
        ++asset_type_index;
    } else if (asset_type_index == serializer::serializable_assets) {
        task_progress = 1.f;
        task_status = "Done!";
        window_list_menu::delete_entry(&task_renderer<task_type>);
        // Reset state for next time
        asset_type_index = -1;
        if (task_end_callback) {
            task_end_callback();
            task_end_callback = nullptr;
        }
    } else {
        const auto set_progress_vars = [task_type_str](std::string_view progress_str,
                                                       float progress) {
            task_progress = progress;
            task_status = std::string(task_type_str) + " " + std::string(progress_str) + "...";
        };

        if constexpr (task_type == TaskType::saving)
            serializer::save_one_asset_type(static_cast<std::size_t>(asset_type_index),
                                            project_path, set_progress_vars);
        else
            serializer::load_one_asset_type(static_cast<std::size_t>(asset_type_index),
                                            project_path, set_progress_vars);

        ++asset_type_index;
    }

    ImGui::OpenPopup(task_type_str.data());
    ImGui::SetNextWindowSize(ImVec2{300, 0}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(task_type_str.data(), nullptr,
                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted(task_status.c_str());
        ImGui::ProgressBar(task_progress);
        ImGui::EndPopup();
    }
}

void start_save() {
    if (project_path.empty() || !fs::is_directory(project_path)) {
        if (const char* c_path =
                noc_file_dialog_open(NOC_FILE_DIALOG_DIR, nullptr, nullptr, nullptr)) {
            project_path = c_path;
            if (!fs::is_directory(project_path)) {
                project_path = project_path.parent_path();
            }
        } else
            return;
    }

    window_list_menu::add_entry({"", &task_renderer<TaskType::saving>, true, false});
}

void start_load(fs::path _project_path, bool ignore_editor_version) {
    ProjectFileData data = load_project_file(_project_path);
    if (!ignore_editor_version && data.editor_version != ARPIYI_EDITOR_VERSION) {
        throw exceptions::EditorVersionDiffers(_project_path, data.editor_version);
    }
    global_tile_size::set(data.tile_size);

    project_path = _project_path;
    window_list_menu::add_entry({"", &task_renderer<TaskType::loading>, true, false});
}
void set_callback(std::function<void()> cb) { task_end_callback = cb; }

} // namespace arpiyi::serializing_manager