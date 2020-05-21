#include "assets/sprite.hpp"

#include <fstream>
#include <string_view>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi::assets {

math::Rect2D Sprite::bounds() const {
    math::Rect2D rect{{0, 0}, {0, 0}};
    for (const auto& piece : pieces) {
        const math::Rect2D piece_bounds{
            piece.destination.start - pivot,
            piece.destination.end - pivot
        };
        if (piece_bounds.start.x < rect.start.x)
            rect.start.x = piece_bounds.start.x;
        if (piece_bounds.start.y < rect.start.y)
            rect.start.y = piece_bounds.start.y;
        if (piece_bounds.end.x > rect.end.x)
            rect.end.x = piece_bounds.end.x;
        if (piece_bounds.end.y > rect.end.y)
            rect.end.y = piece_bounds.end.y;
    }
    return rect;
}

namespace sprite_file_definitions {

constexpr std::string_view texture_id_json_key = "texture";
constexpr std::string_view pieces_json_key = "pieces";
constexpr std::string_view pivot_json_key = "pivot";

} // namespace sprite_file_definitions

template<> RawSaveData raw_get_save_data(Sprite const& sprite) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace sprite_file_definitions;

    w.StartObject();
    {
        w.Key(texture_id_json_key.data());
        w.Uint64(static_cast<u64>(sprite.texture.get_id()));

        w.Key(pieces_json_key.data());
        w.StartArray();
        for (const auto& piece : sprite.pieces) {
            w.StartObject();
            w.Key("source");
            w.StartArray();
            w.Double(piece.source.start.x);
            w.Double(piece.source.start.y);
            w.Double(piece.source.end.x);
            w.Double(piece.source.end.y);
            w.EndArray();
            w.Key("destination");
            w.StartArray();
            w.Double(piece.destination.start.x);
            w.Double(piece.destination.start.y);
            w.Double(piece.destination.end.x);
            w.Double(piece.destination.end.y);
            w.EndArray();
            w.EndObject();
        }
        w.EndArray();

        w.Key(pivot_json_key.data());
        w.StartArray();
        w.Double(sprite.pivot.x);
        w.Double(sprite.pivot.y);
        w.EndArray();
    }
    w.EndObject();
    {
        RawSaveData data;
        data.bytestream.write(s.GetString(), s.GetLength());
        return data;
    }
}

template<> void raw_load(Sprite& sprite, LoadParams<Sprite> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    using namespace sprite_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == pieces_json_key.data()) {
            for (const auto& p_arr_obj : obj.value.GetArray()) {
                auto& p = sprite.pieces.emplace_back();
                const auto& piece = p_arr_obj.GetObject();
                p.source.start.x = piece["source"].GetArray()[0].GetFloat();
                p.source.start.y = piece["source"].GetArray()[1].GetFloat();
                p.source.end.x = piece["source"].GetArray()[2].GetFloat();
                p.source.end.y = piece["source"].GetArray()[3].GetFloat();

                p.destination.start.x = piece["destination"].GetArray()[0].GetFloat();
                p.destination.start.y = piece["destination"].GetArray()[1].GetFloat();
                p.destination.end.x = piece["destination"].GetArray()[2].GetFloat();
                p.destination.end.y = piece["destination"].GetArray()[3].GetFloat();
            }
        } else if (obj.name == pivot_json_key.data()) {
            sprite.pivot = {obj.value.GetArray()[0].GetFloat(), obj.value.GetArray()[1].GetFloat()};
        } else if (obj.name == texture_id_json_key.data()) {
            sprite.texture = Handle<assets::TextureAsset>(obj.value.GetUint64());
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi::assets