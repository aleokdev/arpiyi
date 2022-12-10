#ifndef ARPIYI_ENTITY_HPP
#define ARPIYI_ENTITY_HPP

#include "asset.hpp"
#include "asset_manager.hpp"
#include "script.hpp"
#include "sprite.hpp"

#include <filesystem>
#include <string>
#include <vector>

#include <anton/math/vector2.hpp>

namespace fs = std::filesystem;
namespace aml = anton::math;

namespace arpiyi::assets {

struct [[assets::serialize]] [[assets::load_before(Script)]] [[meta::dir_name("entities")]] Entity {
    std::string name;
    Handle<Sprite> sprite;
    /// Position of this entity (measured in tiles).
    aml::Vector2 pos;
    std::vector<Handle<assets::Script>> scripts;

    /// @returns The origin position of the entity.
    [[nodiscard]] aml::Vector2 get_origin_pos() const {
        if (auto s = sprite.get()) {
            const auto& spr = *s;
            assert(spr.texture.get());
            const auto& texture = *spr.texture.get();

            return {pos.x - texture.w * spr.pivot.x, pos.y - texture.h * spr.pivot.y};
        } else {
            return {pos.x, pos.y};
        }
    }

    static constexpr u64 name_length_limit = 32;
};

template<> struct LoadParams<Entity> { fs::path path; };

template<> RawSaveData raw_get_save_data<Entity>(Entity const&);
template<> void raw_load<Entity>(Entity&, LoadParams<Entity> const& params);

} // namespace arpiyi_editor::assets

#endif // ARPIYI_ENTITY_HPP
