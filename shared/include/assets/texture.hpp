#ifndef ARPIYI_EDITOR_TEXTURE_HPP
#define ARPIYI_EDITOR_TEXTURE_HPP

#include "asset.hpp"

#include <filesystem>

#include "util/intdef.hpp"

#include <imgui.h>
#include <anton/math/vector4.hpp>

namespace aml = anton::math;
namespace fs = std::filesystem;

namespace arpiyi::assets {

struct [[assets::serialize]] [[meta::dir_name("textures")]] Texture {
    u32 w;
    u32 h;

    /// Defined in renderer/renderer_impl_xxx.cpp
    ImTextureID get_imgui_id();
    /// Defined in renderer/renderer_impl_xxx.cpp
    bool exists();
    void* handle;
};

enum class TextureFilter { point, linear };
template<> struct LoadParams<Texture> {
    fs::path path;
    bool flip = false;
    TextureFilter filter = TextureFilter::point;
};

/// Defined in renderer/renderer_impl_xxx.cpp
template<> void raw_load(Texture& texture, LoadParams<Texture> const& params);

/// Defined in renderer/renderer_impl_xxx.cpp
template<> RawSaveData raw_get_save_data<Texture>(Texture const& texture);

/// Defined in renderer/renderer_impl_xxx.cpp
template<> void raw_unload(Texture& texture);

} // namespace arpiyi_editor::assets

#endif // ARPIYI_EDITOR_TEXTURE_HPP