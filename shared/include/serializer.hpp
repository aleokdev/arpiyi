#ifndef ARPIYI_SERIALIZER_HPP
#define ARPIYI_SERIALIZER_HPP

#include "asset_manager.hpp"

#include <fstream>
#include <functional>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi::serializer {

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

template<typename AssetT>
void load_assets(fs::path const& project_path,
                 std::function<void(std::string_view /* progress string */,
                                    float /* progress (0~1) */)> const& per_step_func) {
    namespace mfd = detail::meta_file_definitions;
    std::string meta_filename = assets::AssetDirName<AssetT>::value.data();
    meta_filename += ".json";
    if (!fs::exists(project_path / detail::project_file_definitions::metadata_path / meta_filename))
        return;
    // Read meta document
    std::ifstream f(project_path / detail::project_file_definitions::metadata_path / meta_filename);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    std::size_t i = 0;
    for (auto const& asset_meta : doc.GetArray()) {
        const auto id = asset_meta.GetObject()[mfd::id_json_key.data()].GetUint64();
        per_step_func(assets::AssetDirName<AssetT>::value,
                      static_cast<float>(i) / static_cast<float>(doc.GetArray().Size()));
        const fs::path path =
            project_path / asset_meta.GetObject()[mfd::path_json_key.data()].GetString();
        AssetT asset;
        assets::raw_load(asset, {path});
        asset_manager::put(asset, id);
        ++i;
    }
};

template<typename AssetT>
void save_assets(fs::path const& project_path,
                 std::function<void(std::string_view /* progress string */,
                                    float /* progress (0~1) */)> const& per_step_func) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> meta(s);
    namespace pfd = detail::project_file_definitions;
    namespace mfd = detail::meta_file_definitions;

    const auto& container = ::arpiyi::detail::AssetContainer<AssetT>::get_instance();

    std::size_t i = 0;
    meta.StartArray();
    for (auto const& [id, asset] : container.map) {
        const std::string asset_filename = std::to_string(id) + ".asset";
        const fs::path asset_path =
            project_path / assets::AssetDirName<AssetT>::value / asset_filename;
        fs::create_directories(asset_path.parent_path());
        assets::RawSaveData data = assets::raw_get_save_data(asset);
        std::ofstream f(asset_path);
        f << data.bytestream.rdbuf();

        // Add metadata object
        meta.StartObject();
        meta.Key(mfd::id_json_key.data());
        meta.Uint64(id);
        meta.Key(mfd::path_json_key.data());
        std::string const relative_path = fs::relative(asset_path, project_path).generic_string();
        meta.String(relative_path.c_str());
        meta.EndObject();

        per_step_func(relative_path,
                      1.f / static_cast<float>(container.map.size()) * static_cast<float>(i));
        ++i;
    }
    meta.EndArray();

    {
        std::string meta_filename = assets::AssetDirName<AssetT>::value.data();
        meta_filename += ".json";
        fs::create_directories(project_path / pfd::metadata_path);
        std::ofstream meta_file(project_path / pfd::metadata_path / meta_filename);
        meta_file << s.GetString();
    }
}

/// Loads all assets marked with the [[assets::serialize]] attribute into their respective
/// containers. Defined in the generated build/src/serializer_cg.cpp file.
void load_all_assets(fs::path project_path,
                     std::function<void(std::string_view /* progress string */,
                                        float /* progress (0~1) */)> per_step_func);
/// Saves all assets marked with the [[assets::serialize]] attribute to a path. Defined in the
/// generated build/src/serializer_cg.cpp file.
void save_all_assets(fs::path project_path,
                     std::function<void(std::string_view /* progress string */,
                                        float /* progress (0~1) */)> per_step_func);

} // namespace arpiyi::serializer

#endif // ARPIYI_SERIALIZER_HPP
