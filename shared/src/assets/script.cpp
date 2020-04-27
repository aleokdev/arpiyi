#include "assets/script.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>
#include <rapidjson/document.h>

namespace arpiyi::assets {

constexpr std::string_view name_json_key = "name";
constexpr std::string_view source_json_key = "source";
constexpr std::string_view trigger_type_json_key = "trigger_type";


template<> RawSaveData raw_get_save_data<Script>(Script const& script) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);
    RawSaveData data;

    w.StartObject();
    w.Key(name_json_key.data());
    w.String(script.name.c_str());
    w.Key(source_json_key.data());
    w.String(script.source.c_str());
    w.Key(trigger_type_json_key.data());
    w.Int(static_cast<int>(script.trigger_type));
    w.EndObject();

    data.bytestream.write(s.GetString(), s.GetLength());
    return data;
}

template<> void raw_load<Script>(Script& script, LoadParams<Script> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    for(const auto& node : doc.GetObject()) {
        if(node.name == name_json_key.data()) {
            script.name = node.value.GetString();
        } else if(node.name == source_json_key.data()) {
            script.source = node.value.GetString();
        } else if(node.name == trigger_type_json_key.data()) {
            script.trigger_type = static_cast<Script::TriggerType>(node.value.GetInt());
        }
    }
}

}