#include "serializing_manager.hpp"
#include "assets/map.hpp"
#include "project_info.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <imgui.h>
#include <mutex>
#include <noc_file_dialog.h>
#include <thread>

namespace fs = std::filesystem;

namespace arpiyi_editor::serializing_manager {

namespace detail::meta_file_definitions {

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
        if (obj.name == editor_version_json_key.data()) {
            file_data.editor_version = obj.value.GetString();
        }
    }

    return file_data;
}

std::atomic<float> task_progress = 0;
bool is_saving = false;
bool is_loading = false;
static fs::path last_project_path;
class protected_string {
public:
    std::string get() {
        mut.lock();
        std::string copy = str;
        mut.unlock();
        return copy;
    }

    void set(std::string const& val) {
        mut.lock();
        str = val;
        mut.unlock();
    }

private:
    std::string str;
    std::mutex mut;
} task_status;

void save(fs::path project_save_path) {
    task_status.set("Saving project file...");
    save_project_file(project_save_path);
    constexpr u64 assets_to_save = 4;
    task_progress = 1.f / static_cast<float>(assets_to_save);
    u64 cur_type_loading = 1;

    using namespace ::arpiyi_editor::detail;

    const auto save_assets = [&project_save_path, &cur_type_loading](auto container) {
        std::size_t i = 0;
        for (auto const& [id, asset] : container.map) {
            task_status.set("Saving " +
                            std::string(detail::meta_file_definitions::AssetDirName<
                                        typename std::remove_const<decltype(asset)>::type>::value) +
                            "/" + std::to_string(id) + ".asset...");
            detail::save_asset_file(project_save_path, id, asset);
            task_progress =
                static_cast<float>(cur_type_loading) / static_cast<float>(assets_to_save) +
                1.f / static_cast<float>(assets_to_save) *
                    (static_cast<float>(i) / container.map.size());
            ++i;
        }
        ++cur_type_loading;
    };

    save_assets(AssetContainer<assets::Texture>::get_instance());
    save_assets(AssetContainer<assets::Tileset>::get_instance());
    save_assets(AssetContainer<assets::Map>::get_instance());

    task_progress = 1.f;
}

void load(fs::path project_load_path) {
    task_progress = 0.f;
    u64 cur_type_loading = 0;
    constexpr u64 assets_to_save = 3;

    using namespace ::arpiyi_editor::detail;

    const auto load_assets = [&project_load_path, &cur_type_loading](auto container) {
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
            task_status.set(
                "Loading " +
                std::string(detail::meta_file_definitions::AssetDirName<AssetT>::value) + "/" +
                std::to_string(id) + ".asset...");
            AssetT asset;
            assets::raw_load(asset, {path});
            asset_manager::put(asset, id);
            task_progress =
                static_cast<float>(cur_type_loading) / static_cast<float>(assets_to_save) +
                1.f / static_cast<float>(assets_to_save) *
                    (static_cast<float>(i) / doc.GetArray().Size());
            ++i;
        }
        ++cur_type_loading;
    };

    load_assets(AssetContainer<assets::Texture>::get_instance());
    load_assets(AssetContainer<assets::Tileset>::get_instance());
    load_assets(AssetContainer<assets::Map>::get_instance());

    task_progress = 1.f;
}

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
    is_saving = true;
    std::thread task(save, last_project_path);
    task.detach();
}

void start_load(fs::path project_path) {
    ProjectFileData data = load_project_file(project_path);
    if (data.editor_version != ARPIYI_EDITOR_VERSION) {
        // TODO: Do something if editor versions don't match
    }
    is_loading = true;
    last_project_path = project_path;
    std::thread task(load, project_path);
    task.detach();
}

void render() {
    if (is_saving || is_loading) {
        ImGui::OpenPopup(is_saving ? "Saving..." : "Loading...");
        ImGui::SetNextWindowSize(ImVec2{300, 0}, ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal(is_saving ? "Saving..." : "Loading...", nullptr,
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            ImGui::TextUnformatted(task_status.get().c_str());
            ImGui::ProgressBar(task_progress);
            ImGui::EndPopup();

            if (task_progress == 1.f) {
                is_saving = is_loading = false;
                task_progress = 0;
            }
        }
    }
}

} // namespace arpiyi_editor::serializing_manager