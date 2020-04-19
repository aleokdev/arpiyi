#ifndef ARPIYI_TILESET_HPP
#define ARPIYI_TILESET_HPP

#include "asset_manager.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "util/intdef.hpp"
#include "util/math.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace fs = std::filesystem;

namespace arpiyi::assets {

struct Tileset {
    enum class AutoType {
        none,
        // Used for RPGMaker A2 tilesets (Those that only contain floor / floor details).
        // i.e. Inside_A2
        // This will also work with A1 tilesets, but those are meant to be animated.
        rpgmaker_a2,
        count
    } auto_type;

    Handle<assets::Texture> texture;
    std::string name;

    /// Returns the size of this tileset in tiles, taking the tilesize as an argument.
    [[nodiscard]] math::IVec2D get_size_in_tiles() const;

    [[nodiscard]] math::Rect2D get_uv(u32 id) const;

    [[nodiscard]] u32 get_id(math::IVec2D pos) const;
    [[nodiscard]] u32 get_id_auto(u32 x_index, u32 surroundings) const;
    [[nodiscard]] u32 get_surroundings_from_auto_id(u32 id) const;
    [[nodiscard]] u32 get_x_index_from_auto_id(u32 id) const;
};

template<> inline void raw_unload<Tileset>(Tileset& tileset) { tileset.texture.unload(); }

template<> struct LoadParams<Tileset> { fs::path path; };

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset);
template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params);

template<> struct AssetDirName<assets::Tileset> {
    constexpr static std::string_view value = "tilesets";
};

} // namespace arpiyi_editor::assets

#endif // ARPIYI_TILESET_HPP
