#ifndef ARPIYI_EDITOR_TEXTURE_HPP
#define ARPIYI_EDITOR_TEXTURE_HPP

#include "asset.hpp"

#include <filesystem>

#include "util/intdef.hpp"

#include <anton/math/vector4.hpp>
#include <imgui.h>

namespace aml = anton::math;
namespace fs = std::filesystem;

namespace arpiyi::assets {

// TODO: Move to renderer.hpp (Probably call it RawTexture), then make a texture wrapper here that
// contains the source path of the texture and the RawTexture. On save, just copy the source file to
// the save buffer.
// RawTexture should just be used in the texture wrapper and in the renderer implementation, nowhere
// else.
struct [[assets::serialize]] [[meta::dir_name("textures")]] Texture {
    u32 w;
    u32 h;

    /// Defined in renderer/renderer_impl_xxx.cpp
    ImTextureID get_imgui_id();
    /// Defined in renderer/renderer_impl_xxx.cpp
    bool exists();
    // TODO: Use p_impl
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

} // namespace arpiyi::assets

#endif // ARPIYI_EDITOR_TEXTURE_HPP