#include "assets/tileset.hpp"
#include "global_tile_size.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <rapidjson/document.h>
#include <set>

#include "util/defs.hpp"
#include "util/intdef.hpp"

namespace arpiyi::assets {

// TODO: Codegenize this
Sprite Tileset::Tile::full_sprite() const {
    assert(tileset.get());
    switch (tileset.get()->tile_type) {
        case TileType::normal: return impl_full_sprite<TileType::normal>();
        //case TileType::rpgmaker_a2: return impl_full_sprite<TileType::rpgmaker_a2>();
        default: assert(false && "Unknown tileset type"); return {};
    }
};

// TODO: Codegenize this
Sprite Tileset::Tile::preview_sprite() const {
    assert(tileset.get());
    switch (tileset.get()->tile_type) {
        case TileType::normal: return impl_preview_sprite<TileType::normal>();
        //case TileType::rpgmaker_a2: return impl_preview_sprite<TileType::rpgmaker_a2>();
        default: assert(false && "Unknown tileset type"); return {};
    }
};

// TODO: Codegenize this
std::size_t Tileset::tile_count() const {
    switch (tile_type) {
        case TileType::normal: return impl_tile_count<TileType::normal>();
        //case TileType::rpgmaker_a2: return impl_tile_count<TileType::rpgmaker_a2>();
        default: assert(false && "Unknown tileset type"); return {};
    }
}

math::IVec2D Tileset::size_in_tile_units() const {
    assert(texture.get());
    const auto& tex = *texture.get();
    return {static_cast<i32>(tex.w / global_tile_size::get()),
            static_cast<i32>(tex.h / global_tile_size::get())};
}

namespace tileset_file_definitions {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view autotype_json_key = "auto_type";
constexpr std::string_view texture_id_json_key = "texture_id";

} // namespace tileset_file_definitions

template<> RawSaveData raw_get_save_data<Tileset>(Tileset const& tileset) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace tileset_file_definitions;

    w.StartObject();
    {
        w.Key(name_json_key.data());
        w.String(tileset.name.data());

        w.Key(autotype_json_key.data());
        w.Uint64(static_cast<u64>(tileset.tile_type));

        w.Key(texture_id_json_key.data());
        w.Uint64(tileset.texture.get_id());
    }
    w.EndObject();
    {
        RawSaveData data;
        data.bytestream.write(s.GetString(), s.GetLength());
        return data;
    }
}

template<> void raw_load<Tileset>(Tileset& tileset, LoadParams<Tileset> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    using namespace tileset_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == name_json_key.data()) {
            tileset.name = obj.value.GetString();
        } else if (obj.name == autotype_json_key.data()) {
            u32 auto_type = obj.value.GetUint();

            // TODO: Move check to load_tilesets and throw a proper exception
            assert(auto_type >= static_cast<u32>(assets::TileType::normal) &&
                   auto_type < static_cast<u32>(assets::TileType::count));

            tileset.tile_type = static_cast<assets::TileType>(auto_type);
        } else if (obj.name == texture_id_json_key.data()) {
            tileset.texture = Handle<assets::Texture>(obj.value.GetUint64());
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi::assets