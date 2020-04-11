/* clang-format off */
#include <glad/glad.h>
#include <GLFW/glfw3.h>
/* clang-format on */
#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include "serializing_manager.hpp"
#include "tileset_manager.hpp"
#include "assets/map.hpp"
#include "project_info.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <imgui.h>
#include <noc_file_dialog.h>
#include <window_manager.hpp>

namespace fs = std::filesystem;

namespace arpiyi_editor::serializing_manager {

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

/* clang-format off */
template<> struct AssetDirName<assets::Map> {
    constexpr static std::string_view value = "maps";
};

template<> struct AssetDirName<assets::Texture> {
    constexpr static std::string_view value = "textures";
};

template<> struct AssetDirName<assets::Tileset> {
    constexpr static std::string_view value = "tilesets";
};
/* clang-format on */

} // namespace detail::meta_file_definitions

static void save_project_file(fs::path base_dir) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    using namespace detail::project_file_definitions;

    w.StartObject();
    {
        w.Key(tile_size_json_key.data());
        w.Uint(tileset_manager::get_tile_size());
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
        if(obj.name == tile_size_json_key.data()) {
            file_data.tile_size = obj.value.GetUint();
        }
        else if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        }
    }

    return file_data;
}

float task_progress = 0;
static fs::path last_project_path;
std::string task_status;

void save(fs::path project_save_path, std::function<void(void)> per_step) {
    task_status = "Saving project file...";
    save_project_file(project_save_path);
    constexpr u64 assets_to_save = 4;
    task_progress = 1.f / static_cast<float>(assets_to_save);
    u64 cur_type_loading = 1;

    using namespace ::arpiyi_editor::detail;

    const auto save_assets = [&project_save_path, &cur_type_loading, &per_step](auto container) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> meta(s);
        namespace pfd = detail::project_file_definitions;
        namespace mfd = detail::meta_file_definitions;

        using AssetT = typename decltype(container)::AssetType;
        std::size_t i = 0;
        meta.StartArray();
        for (auto const& [id, asset] : container.map) {
            task_status = ("Saving " +
                           std::string(detail::meta_file_definitions::AssetDirName<AssetT>::value) +
                           "/" + std::to_string(id) + ".asset...");

            const fs::path asset_path = project_save_path / detail::get_asset_save_path<AssetT>(id);
            fs::create_directories(asset_path.parent_path());
            assets::RawSaveData data = assets::raw_get_save_data(asset);
            std::ofstream f(asset_path);
            f << data.bytestream.rdbuf();

            // Add metadata object
            meta.StartObject();
            meta.Key(mfd::id_json_key.data());
            meta.Uint64(id);
            meta.Key(mfd::path_json_key.data());
            meta.String(fs::relative(asset_path, project_save_path).c_str());
            meta.EndObject();

            task_progress =
                static_cast<float>(cur_type_loading) / static_cast<float>(assets_to_save) +
                1.f / static_cast<float>(assets_to_save) *
                    (static_cast<float>(i) / container.map.size());
            per_step();
            ++i;
        }
        meta.EndArray();

        {
            std::string meta_filename = mfd::AssetDirName<AssetT>::value.data();
            meta_filename += ".json";
            fs::create_directories(project_save_path / pfd::metadata_path);
            std::ofstream meta_file(project_save_path / pfd::metadata_path / meta_filename);
            meta_file << s.GetString();
        }
        ++cur_type_loading;
    };

    save_assets(AssetContainer<assets::Texture>::get_instance());
    save_assets(AssetContainer<assets::Tileset>::get_instance());
    save_assets(AssetContainer<assets::Map>::get_instance());

    task_progress = 1.f;
}

void load(fs::path project_load_path, std::function<void(void)> per_step) {
    task_progress = 0.f;
    u64 cur_type_loading = 0;
    constexpr u64 assets_to_save = 3;

    using namespace ::arpiyi_editor::detail;

    const auto load_assets = [&project_load_path, &cur_type_loading, &per_step](auto container) {
        using AssetT = typename decltype(container)::AssetType;
        // Read meta document
        namespace mfd = detail::meta_file_definitions;
        std::string meta_filename = mfd::AssetDirName<AssetT>::value.data();
        meta_filename += ".json";
        std::ifstream f(project_load_path / detail::project_file_definitions::metadata_path /
                        meta_filename);
        std::stringstream buffer;
        buffer << f.rdbuf();

        rapidjson::Document doc;
        doc.Parse(buffer.str().data());

        std::size_t i = 0;
        for (auto const& asset_meta : doc.GetArray()) {
            const auto id = asset_meta.GetObject()[mfd::id_json_key.data()].GetUint64();
            const fs::path path =
                project_load_path / asset_meta.GetObject()[mfd::path_json_key.data()].GetString();
            task_status = ("Loading " +
                           std::string(detail::meta_file_definitions::AssetDirName<AssetT>::value) +
                           "/" + std::to_string(id) + ".asset...");
            AssetT asset;
            assets::raw_load(asset, {path});
            asset_manager::put(asset, id);
            task_progress =
                static_cast<float>(cur_type_loading) / static_cast<float>(assets_to_save) +
                1.f / static_cast<float>(assets_to_save) *
                    (static_cast<float>(i) / doc.GetArray().Size());
            per_step();
            ++i;
        }
        ++cur_type_loading;
    };

    load_assets(AssetContainer<assets::Texture>::get_instance());
    load_assets(AssetContainer<assets::Tileset>::get_instance());
    load_assets(AssetContainer<assets::Map>::get_instance());

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

void start_load(fs::path project_path) {
    ProjectFileData data = load_project_file(project_path);
    if (data.editor_version != ARPIYI_EDITOR_VERSION) {
        // TODO: Do something if editor versions don't match
    }
    last_project_path = project_path;
    tileset_manager::set_tile_size(data.tile_size);

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

} // namespace arpiyi_editor::serializing_manager