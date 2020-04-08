#ifndef ARPIYI_SPRITE_HPP
#define ARPIYI_SPRITE_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "texture.hpp"
#include "util/math.hpp"

#include <glm/vec2.hpp>

namespace arpiyi_editor::assets {

struct Sprite {
    /// Texture of the sprite. Not owned by it
    Handle<assets::Texture> texture;
    glm::vec2 uv_min;
    glm::vec2 uv_max;
    std::string name;

    [[nodiscard]] math::IVec2D get_size_in_pixels() const {
        const auto tex = *texture.get();
        return {static_cast<i32>((uv_max.x - uv_min.x) * tex.w),
                static_cast<i32>((uv_max.y - uv_min.y) * tex.h)};
    }
};

}

#endif // ARPIYI_SPRITE_HPP
