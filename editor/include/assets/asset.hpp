#ifndef ARPIYI_EDITOR_ASSET_HPP
#define ARPIYI_EDITOR_ASSET_HPP

namespace arpiyi_editor::assets {
template<typename T> struct LoadParams;
template<typename T> struct SaveParams;

template<typename AssetT>
void raw_load(AssetT&, LoadParams<AssetT> const& params);

template<typename AssetT>
void raw_unload(AssetT&);

template<typename AssetT>
void raw_save(AssetT&, SaveParams<AssetT> const& params);

}

#endif // ARPIYI_EDITOR_ASSET_HPP