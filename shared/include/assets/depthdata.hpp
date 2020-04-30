#ifndef ARPIYI_DEPTHDATA_HPP
#define ARPIYI_DEPTHDATA_HPP

#include <vector>
#include "util/math.hpp"
#include "assets/asset.hpp"
#include "util/intdef.hpp"

namespace arpiyi::assets {

enum class TexelType {
    floor = 0,
    wall = 1
};

struct [[meta::dir_name("depthdata")]] DepthData {
    DepthData();

    TexelType get_texel(math::IVec2D pos);
    void set_texel(math::IVec2D pos, TexelType texel);
private:
    const std::size_t tile_size_stored;
    std::vector<u8> data;

    friend RawSaveData raw_get_save_data<DepthData>(DepthData const&);
    friend void raw_load<DepthData>(DepthData&, LoadParams<DepthData> const&);
};

template<> inline void raw_unload<DepthData>(DepthData&) {}

template<> struct LoadParams<DepthData> { fs::path path; };

template<> RawSaveData raw_get_save_data<DepthData>(DepthData const&);
template<> void raw_load<DepthData>(DepthData&, LoadParams<DepthData> const&);

}

#endif // ARPIYI_DEPTHDATA_HPP
