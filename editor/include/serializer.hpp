#ifndef ARPIYI_SERIALIZER_HPP
#define ARPIYI_SERIALIZER_HPP

#include "asset_manager.hpp"
#include "assets/asset.hpp"

#include <filesystem>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

namespace arpiyi_editor::serializer {

namespace project_file_definitions {

constexpr std::string_view default_tilesets_path = "tilesets";
constexpr std::string_view default_maps_path = "maps";
/// Path for storing files containing asset IDs and their location.
constexpr std::string_view metadata_path = "meta";

constexpr std::string_view editor_version_json_key = "editor_version";

} // namespace project_file_definitions

namespace meta_file_definitions {

constexpr std::string_view id_json_key = "id";
constexpr std::string_view path_json_key = "path";

template<typename AssetT> struct AssetDirName;

} // namespace meta_file_definitions

void save_project(fs::path dir);
void load_project(fs::path dir);

template<typename AssetT> void save_asset_files(fs::path base_dir) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> meta(s);
    using namespace project_file_definitions;
    namespace mfd = meta_file_definitions;

    meta.StartArray();
    for (const auto& [id, asset] : detail::AssetContainer<AssetT>::get_instance().map) {
        const std::string asset_filename = std::to_string(id) + ".asset";
        fs::create_directories(base_dir / mfd::AssetDirName<AssetT>::value);
        const fs::path map_path = base_dir / mfd::AssetDirName<AssetT>::value / asset_filename;
        assets::raw_save(asset, {map_path});

        // Save metadata
        meta.StartObject();
        meta.Key(mfd::id_json_key.data());
        meta.Uint64(id);
        meta.Key(mfd::path_json_key.data());
        meta.String(fs::relative(map_path, base_dir).c_str());
        meta.EndObject();
    }
    meta.EndArray();

    {
        std::string meta_filename = mfd::AssetDirName<AssetT>::value.data();
        meta_filename += ".json";
        fs::create_directories(base_dir / metadata_path);
        std::ofstream meta_file(base_dir / metadata_path / meta_filename);
        meta_file << s.GetString();
    }
}

template<typename AssetT> void load_asset_files(fs::path base_dir) {
    // Read meta document
    namespace mfd = meta_file_definitions;
    std::string meta_filename = mfd::AssetDirName<AssetT>::value.data();
    meta_filename += ".json";
    std::ifstream f(base_dir / project_file_definitions::metadata_path / meta_filename);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    for (auto const& asset_meta : doc.GetArray()) {
        AssetT asset;
        assets::raw_load(
            asset, {base_dir / asset_meta.GetObject()[mfd::path_json_key.data()].GetString()});
        asset_manager::put(asset, asset_meta.GetObject()[mfd::id_json_key.data()].GetUint64());
    }
}

} // namespace arpiyi_editor::serializer

#endif // ARPIYI_SERIALIZER_HPP
