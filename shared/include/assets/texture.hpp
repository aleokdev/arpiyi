#ifndef ARPIYI_EDITOR_TEXTURE_HPP
#define ARPIYI_EDITOR_TEXTURE_HPP

#include "asset.hpp"

#include <filesystem>

#include "renderer/renderer.hpp"
#include "util/intdef.hpp"

#include <anton/math/vector4.hpp>
#include <imgui.h>

namespace aml = anton::math;
namespace fs = std::filesystem;

namespace arpiyi::assets {

/// This struct represents a savable (and loadable) color texture. The only difference with
/// renderer::Texture is that this struct also saves the loaded texture path. If you want to
/// represent a texture that is generated or one that won't be saved, you are free to use renderer::Texture
/// instead.
struct [[assets::serialize]] [[meta::dir_name("textures")]] TextureAsset {
    fs::path load_path;
    renderer::TextureHandle handle;
};

template<> struct LoadParams<TextureAsset> {
    fs::path path;
    bool flip = false;
    renderer::TextureHandle::FilteringMethod filter = renderer::TextureHandle::FilteringMethod::point;
};

template<> void raw_load(TextureAsset& texture, LoadParams<TextureAsset> const& params);

template<> RawSaveData raw_get_save_data<TextureAsset>(TextureAsset const& texture);

template<> void raw_unload(TextureAsset& texture);

} // namespace arpiyi::assets

#endif // ARPIYI_EDITOR_TEXTURE_HPP