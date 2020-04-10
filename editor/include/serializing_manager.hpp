#ifndef ARPIYI_SERIALIZING_MANAGER_HPP
#define ARPIYI_SERIALIZING_MANAGER_HPP

#include <filesystem>
#include <string>

#include "asset_manager.hpp"
#include "assets/asset.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

namespace arpiyi_editor::serializing_manager {

namespace detail {

namespace meta_file_definitions {

template<typename AssetT> struct AssetDirName;

} // namespace meta_file_definitions

/// @returns A path relative to the project base directory pointing to where the asset should be
/// saved. Includes filename.
template<typename AssetT> fs::path get_asset_save_path(u64 handle_id) {
    namespace mfd = meta_file_definitions;

    const std::string asset_filename = std::to_string(handle_id) + ".asset";
    return fs::path(mfd::AssetDirName<AssetT>::value) / asset_filename;
}

} // namespace detail

void start_save();
void start_load(fs::path project_path);

void render();

} // namespace arpiyi_editor::serializing_manager

#endif // ARPIYI_SERIALIZING_MANAGER_HPP
