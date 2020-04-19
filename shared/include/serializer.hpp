#ifndef ARPIYI_SERIALIZER_HPP
#define ARPIYI_SERIALIZER_HPP

#include "asset_manager.hpp"

#include <functional>
#include <rapidjson/document.h>

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
void load_assets(fs::path project_path,
                 std::function<void(std::string_view /* progress string */, float /* progress (0~1) */)>
                     per_step_func) {
    namespace mfd = detail::meta_file_definitions;
    std::string meta_filename = assets::AssetDirName<AssetT>::value.data();
    meta_filename += ".json";
    if (!fs::exists(project_path / detail::project_file_definitions::metadata_path /
                    meta_filename))
        return;
    // Read meta document
    std::ifstream f(project_path / detail::project_file_definitions::metadata_path /
                    meta_filename);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    std::size_t i = 0;
    for (auto const& asset_meta : doc.GetArray()) {
        const auto id = asset_meta.GetObject()[mfd::id_json_key.data()].GetUint64();
        per_step_func(assets::AssetDirName<AssetT>::value, static_cast<float>(i) / static_cast<float>(doc.GetArray().Size()));
        const fs::path path =
            project_path / asset_meta.GetObject()[mfd::path_json_key.data()].GetString();
        AssetT asset;
        assets::raw_load(asset, {path});
        asset_manager::put(asset, id);
        ++i;
    }
};

} // namespace arpiyi::serializer

#endif // ARPIYI_SERIALIZER_HPP
