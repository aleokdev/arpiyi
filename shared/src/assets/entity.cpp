#include "assets/entity.hpp"

#include <fstream>
#include <string_view>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi::assets {

namespace entity_file_definitions {
constexpr std::string_view name_json_key = "name";
constexpr std::string_view sprite_id_json_key = "sprite";
constexpr std::string_view pos_json_key = "pos";
constexpr std::string_view scripts_json_key = "scripts";
} // namespace entity_file_definitions

template<> RawSaveData raw_get_save_data<Entity>(Entity const& entity) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    using namespace entity_file_definitions;

    w.StartObject();
    {
        w.Key(name_json_key.data());
        w.String(entity.name.data());

        w.Key(sprite_id_json_key.data());
        w.Uint64(entity.sprite.get_id());

        w.Key(pos_json_key.data());
        w.StartArray();
        w.Double(entity.pos.x);
        w.Double(entity.pos.y);
        w.EndArray();

        w.Key(scripts_json_key.data());
        w.StartArray();
        for (const auto& script : entity.scripts) { w.Uint64(script.get_id()); }
        w.EndArray();
    }
    w.EndObject();
    {
        RawSaveData data;
        data.bytestream.write(s.GetString(), s.GetLength());
        return data;
    }
}

template<> void raw_load<Entity>(Entity& entity, LoadParams<Entity> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    using namespace entity_file_definitions;

    for (auto const& obj : doc.GetObject()) {
        if (obj.name == name_json_key.data()) {
            entity.name = obj.value.GetString();
        } else if (obj.name == sprite_id_json_key.data()) {
            entity.sprite = obj.value.GetUint64();
        } else if (obj.name == pos_json_key.data()) {
            entity.pos.x = obj.value.GetArray()[0].GetFloat();
            entity.pos.y = obj.value.GetArray()[1].GetFloat();
        } else if (obj.name == scripts_json_key.data()) {
            for (const auto& script_id : obj.value.GetArray()) {
                entity.scripts.emplace_back(script_id.GetUint64());
            }
        } else
            assert("Unknown JSON key in tileset file");
    }
}

} // namespace arpiyi_editor::assets