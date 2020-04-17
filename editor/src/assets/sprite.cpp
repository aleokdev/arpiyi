#include "assets/sprite.hpp"

#include <string_view>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi_editor::assets {

namespace sprite_file_definitions {

constexpr std::string_view texture_id_json_key = "texture";
constexpr std::string_view uv_min_json_key = "uv_min";
constexpr std::string_view uv_max_json_key = "uv_max";
constexpr std::string_view name_json_key = "name";
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

        w.Key(uv_min_json_key.data());
        w.StartArray();
        w.Double(sprite.uv_min.x);
        w.Double(sprite.uv_min.y);
        w.EndArray();

        w.Key(uv_max_json_key.data());
        w.StartArray();
        w.Double(sprite.uv_max.x);
        w.Double(sprite.uv_max.y);
        w.EndArray();

        w.Key(name_json_key.data());
        w.String(sprite.name.data());

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
        if (obj.name == name_json_key.data()) {
            sprite.name = obj.value.GetString();
        } else if (obj.name == uv_min_json_key.data()) {
            sprite.uv_min = {obj.value.GetArray()[0].GetFloat(), obj.value.GetArray()[1].GetFloat()};
        } else if (obj.name == uv_max_json_key.data()) {
            sprite.uv_max = {obj.value.GetArray()[0].GetFloat(), obj.value.GetArray()[1].GetFloat()};
        } else if (obj.name == pivot_json_key.data()) {
            sprite.pivot = {obj.value.GetArray()[0].GetFloat(), obj.value.GetArray()[1].GetFloat()};
        } else if (obj.name == texture_id_json_key.data()) {
            sprite.texture = obj.value.GetUint64();
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi_editor::assets