#include <glad/glad.h>
// GLAD must be included before GLFW
#include <GLFW/glfw3.h>

#include <imgui.h>
// imgui.h must be included before the API implementations
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <noc_file_dialog.h>

#include "assets/entity.hpp"
#include "assets/map.hpp"
#include "assets/script.hpp"
#include "assets/sprite.hpp"
#include "global_tile_size.hpp"
#include "project_info.hpp"
#include "serializing_exceptions.hpp"
#include "serializing_manager.hpp"
#include "tileset_manager.hpp"
#include "serializer.hpp"

#include "global_tile_size.hpp"
#include "window_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace arpiyi::serializing_manager {

namespace detail::project_file_definitions {

constexpr std::string_view default_tilesets_path = "tilesets";
constexpr std::string_view default_maps_path = "maps";
/// Path for storing files containing asset IDs and their location.
constexpr std::string_view metadata_path = "meta";

constexpr std::string_view tile_size_json_key = "tile_size";
constexpr std::string_view editor_version_json_key = "editor_version";

} // namespace detail::project_file_definitions

namespace detail::meta_file_definitions {

constexpr std::string_view id_json_key = "id";
constexpr std::string_view path_json_key = "path";

} // namespace detail::meta_file_definitions

static void save_project_file(fs::path base_dir) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace detail::project_file_definitions;

    w.StartObject();
    {
        w.Key(tile_size_json_key.data());
        w.Uint(global_tile_size::get());
        w.Key(editor_version_json_key.data());
        w.String(ARPIYI_EDITOR_VERSION);
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
        }
    }

    return file_data;
}

float task_progress = 0;
static fs::path last_project_path;
std::string task_status;

void save(fs::path project_save_path, std::function<void(void)> per_step) {
    task_progress = 0.f;
    task_status = "Saving project file...";
    save_project_file(project_save_path);

    const auto wrapped_per_step = [&per_step](std::string_view progress_str, float progress) {
      task_progress = progress;
      task_status = progress_str;
      per_step();
    };

    serializer::load_all_assets(project_save_path, wrapped_per_step);

    task_progress = 1.f;
}

void load(fs::path project_load_path, std::function<void(void)> per_step) {
    task_progress = 0.f;
    task_status = "Loading project file...";
    load_project_file(project_load_path);

    const auto wrapped_per_step = [&per_step](std::string_view progress_str, float progress) {
        task_progress = progress;
        task_status = progress_str;
        per_step();
    };

    serializer::load_all_assets(project_load_path, wrapped_per_step);

    task_progress = 1.f;
}

enum class DialogType { none, saving, loading };
template<DialogType dialog_type> void per_step_callback() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_manager::get_window());
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::OpenPopup(dialog_type == DialogType::saving ? "Saving..." : "Loading...");
    ImGui::SetNextWindowSize(ImVec2{300, 0}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(dialog_type == DialogType::saving ? "Saving..." : "Loading...",
                               nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::TextUnformatted(task_status.c_str());
        ImGui::ProgressBar(task_progress);
        ImGui::EndPopup();
    }

    glfwPollEvents();

    int display_w, display_h;
    glfwGetFramebufferSize(window_manager::get_window(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

static DialogType dialog_to_render = DialogType::none;
void start_save() {
    if (last_project_path.empty() || !fs::is_directory(last_project_path)) {
        if (const char* c_path =
                noc_file_dialog_open(NOC_FILE_DIALOG_DIR, nullptr, nullptr, nullptr)) {
            last_project_path = c_path;
            if (!fs::is_directory(last_project_path)) {
                last_project_path = last_project_path.parent_path();
            }
        } else
            return;
    }
    dialog_to_render = DialogType::saving;
}

void start_load(fs::path project_path, bool ignore_editor_version) {
    ProjectFileData data = load_project_file(project_path);
    if (!ignore_editor_version && data.editor_version != ARPIYI_EDITOR_VERSION) {
        throw exceptions::EditorVersionDiffers(project_path, data.editor_version);
    }
    last_project_path = project_path;
    global_tile_size::set(data.tile_size);

    dialog_to_render = DialogType::loading;
}

// Opening the dialogs is the LAST thing that should be done in the frame (imgui
// destroys layout in some specific conditions), so load/save is not called
// directly on start_x, but rather on render().
void render() {
    switch (dialog_to_render) {
        case DialogType::loading:
            per_step_callback<DialogType::loading>();
            load(last_project_path, per_step_callback<DialogType::loading>);
            dialog_to_render = DialogType::none;
            break;
        case DialogType::saving:
            per_step_callback<DialogType::saving>();
            save(last_project_path, per_step_callback<DialogType::saving>);
            dialog_to_render = DialogType::none;
            break;
        default: break;
    }
}

} // namespace arpiyi::serializing_manager