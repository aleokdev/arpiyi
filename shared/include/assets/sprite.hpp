#ifndef ARPIYI_SPRITE_HPP
#define ARPIYI_SPRITE_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "texture.hpp"
#include "util/math.hpp"

#include <anton/math/vector2.hpp>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
// Anton math library
namespace aml = anton::math;

namespace arpiyi::assets {

struct TextureChunk {
    renderer::TextureHandle tex;
    math::Rect2D rect;
};

struct [[assets::serialize]] [[meta::dir_name("sprites")]] Sprite {
    /// Texture of the sprite. Not owned by it
    Handle<assets::TextureAsset> texture;
    struct Piece {
        /// Where this piece is gathering texture data from, in UV coordinates.
        math::Rect2D source;
        /// The destination of the source texture. Measured in tiles.
        math::Rect2D destination;
    };
    /// The "pieces" that make up this sprite. A sprite is basically a puzzle of different pieces,
    /// each one having its own texture UV source and destination rect.
    std::vector<Piece> pieces;
    /// Where this sprite originates from, in tile units. i.e. {0,0} will not move any of the pieces
    /// when creating a mesh, but {1,0} will move all of the pieces 1 unit left. Consider this the
    /// "view position" of the sprite. This is only really needed for sprite rotation and
    /// fine-tuning.
    aml::Vector2 pivot = {0, 0};

    /// @returns A rect containing all the pieces (destination rects) of the sprite.
    [[nodiscard]] math::Rect2D bounds() const;
};

template<> struct LoadParams<Sprite> { fs::path path; };

template<> RawSaveData raw_get_save_data(Sprite const&);
template<> void raw_load(Sprite&, LoadParams<Sprite> const&);

} // namespace arpiyi::assets

#endif // ARPIYI_SPRITE_HPP
