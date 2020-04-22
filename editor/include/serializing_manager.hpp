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

namespace arpiyi::serializing_manager {

namespace detail {

/// @returns A path relative to the project base directory pointing to where the asset should be
/// saved. Includes filename.
template<typename AssetT> fs::path get_asset_save_path(u64 handle_id) {
    const std::string asset_filename = std::to_string(handle_id) + ".asset";
    return fs::path(assets::AssetDirName<AssetT>::value) / asset_filename;
}

} // namespace detail

void start_save();
void start_load(fs::path project_path, bool ignore_editor_version = false);

void render();

} // namespace arpiyi::serializing_manager

#endif // ARPIYI_SERIALIZING_MANAGER_HPP
