#ifndef ARPIYI_ASSET_MANAGER_HPP
#define ARPIYI_ASSET_MANAGER_HPP

#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

namespace arpiyi_editor::assets {

struct Bitmap {
    unsigned int handler;
};

template<typename T> struct LoadParams;

template<>
struct LoadParams<Bitmap> {
    fs::path texture_path;
};

void raw_load(Bitmap&, LoadParams<Bitmap> const&);

}

namespace arpiyi_editor::asset_manager {

namespace detail {


template<typename AssetT>
struct AssetContainer {
    std::unordered_map<std::size_t, AssetT> map;
    std::size_t last_id;

    static AssetContainer& get_instance() {
        static AssetContainer<AssetT> instance;
        return instance;
    }
};

}

template<typename AssetT>
class Asset {
public:
    Asset(std::size_t id) : id(id) {}
    std::optional<AssetT&> get() {

    }

private:
    std::size_t id;
};

template<typename AssetT>
Asset<AssetT> load(assets::LoadParams<AssetT>&& load_params) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    std::size_t id_to_use = container.last_id++;
    AssetT& asset = container.map.emplace(id_to_use, std::forward(load_params)).first->second;
    assets::raw_load(asset, load_params);
    return Asset<AssetT>(id_to_use);
}

}

#endif // ARPIYI_ASSET_MANAGER_HPP
