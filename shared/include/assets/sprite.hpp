#ifndef ARPIYI_SPRITE_HPP
#define ARPIYI_SPRITE_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "texture.hpp"
#include "util/math.hpp"

#include <anton/math/vector2.hpp>
#include <filesystem>

namespace fs = std::filesystem;
// Anton math library
namespace aml = anton::math;

namespace arpiyi::assets {

struct [[assets::serialize]] [[meta::dir_name("sprites")]] Sprite {
    /// Texture of the sprite. Not owned by it
    Handle<assets::Texture> texture;
    aml::Vector2 uv_min;
    aml::Vector2 uv_max;
    std::string name;
    /// Where this sprite originates from. Goes from {0,0} (Bottom left) to {1,1} (Upper right).
    aml::Vector2 pivot = {0,0};

    [[nodiscard]] math::IVec2D get_size_in_pixels() const {
        const auto tex = *texture.get();
        return {static_cast<i32>((uv_max.x - uv_min.x) * tex.w),
                static_cast<i32>((uv_max.y - uv_min.y) * tex.h)};
    }
};

template<> struct LoadParams<Sprite> {
    fs::path path;
};

template<> RawSaveData raw_get_save_data(Sprite const&);
template<> void raw_load(Sprite&, LoadParams<Sprite> const&);

}

#endif // ARPIYI_SPRITE_HPP
