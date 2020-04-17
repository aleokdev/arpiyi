#ifndef ARPIYI_ASSET_HPP
#define ARPIYI_ASSET_HPP
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi::assets {
template<typename T> struct LoadParams;

template<typename AssetT>
void raw_load(AssetT&, LoadParams<AssetT> const& params);

template<typename AssetT>
void raw_unload(AssetT&);

struct RawSaveData {
    std::stringstream bytestream;
};

template<typename AssetT>
RawSaveData raw_get_save_data(AssetT const&);

}

#endif // ARPIYI_EDITOR_ASSET_HPP