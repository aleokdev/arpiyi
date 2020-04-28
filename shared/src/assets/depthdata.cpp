#include "assets/depthdata.hpp"
#include "global_tile_size.hpp"
#include <cassert>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <fstream>

namespace arpiyi::assets {

int ceil_div(int x, int y) {
    return (x + y - 1) / y;
}

DepthData::DepthData() : tile_size_stored(static_cast<int>(global_tile_size::get())) {
    data = std::vector<u8>(ceil_div(tile_size_stored * tile_size_stored, 8));
}

TexelType DepthData::get_texel(math::IVec2D pos) {
    assert(tile_size_stored == global_tile_size::get());
    assert(pos.x >= 0 && pos.y >= 0 && pos.x < tile_size_stored && pos.y < tile_size_stored);

    std::size_t raw_pos = pos.x + pos.y * tile_size_stored;
    return static_cast<TexelType>(data[raw_pos / 8] & (1u << (raw_pos % 8u)));
}

void DepthData::set_texel(math::IVec2D pos, TexelType texel) {
    assert(tile_size_stored == global_tile_size::get());
    assert(pos.x >= 0 && pos.y >= 0 && pos.x < tile_size_stored && pos.y < tile_size_stored);

    std::size_t raw_pos = pos.x + pos.y * tile_size_stored;
    if(texel == TexelType::floor) {
        data[raw_pos / 8] &= 0xFFu ^ (1u << (raw_pos % 8u));
    } else {
        data[raw_pos / 8] |= 1u << (raw_pos % 8u);
    }
}

template<> RawSaveData raw_get_save_data<DepthData>(DepthData const& depthdata) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> w(s);

    w.StartObject();
    w.Key("length");
    w.Uint64(depthdata.data.size());
    w.Key("data");
    w.StartArray();
    for(const auto chunk : depthdata.data) {
        w.Uint(chunk);
    }
    w.EndArray();
    w.EndObject();

    RawSaveData data;
    data.bytestream.write(s.GetString(), s.GetLength());

    return data;
}

template<> void raw_load<DepthData>(DepthData& depthdata, LoadParams<DepthData> const& params) {
    std::ifstream f(params.path);
    std::stringstream buffer;
    buffer << f.rdbuf();

    rapidjson::Document doc;
    doc.Parse(buffer.str().data());

    const auto tile_size_stored = global_tile_size::get();
    assert(doc.GetObject()["length"].GetUint64() == ceil_div(tile_size_stored * tile_size_stored, 8));
    std::size_t i = 0;
    for(const auto& chunk : doc.GetObject()["data"].GetArray()) {
        depthdata.data[i] = chunk.GetUint();
        ++i;
    }
}

}