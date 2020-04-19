#ifndef ARPIYI_SPRITE_HPP
#define ARPIYI_SPRITE_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "texture.hpp"
#include "util/math.hpp"

#include <glm/vec2.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::assets {

struct [[meta::dir_name("sprites")]] Sprite {
    /// Texture of the sprite. Not owned by it
    Handle<assets::Texture> texture;
    glm::vec2 uv_min;
    glm::vec2 uv_max;
    std::string name;
    /// Where this sprite originates from. Goes from {0,0} (Upper left) to {1,1} (Lower right).
    glm::vec2 pivot;

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
