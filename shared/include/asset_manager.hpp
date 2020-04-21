#ifndef ARPIYI_ASSET_MANAGER_HPP
#define ARPIYI_ASSET_MANAGER_HPP

#include "assets/asset.hpp"

#include "util/intdef.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <unordered_map>

namespace arpiyi {

namespace detail {

template<typename AssetT> struct AssetContainer {
    std::unordered_map<std::size_t, AssetT> map;
    u64 next_id_to_use;

    static AssetContainer& get_instance() {
        static AssetContainer<AssetT> instance;
        return instance;
    }

    using AssetType = AssetT;
};

} // namespace detail

template<typename T> class Expected {
public:
    Expected(std::nullptr_t) : val{nullptr}, has_value{false} {}
    Expected(T* val) : val{val}, has_value{true} {}

    T* operator->() {
        if (!has_value)
            assert("Tried to access null Expected value");
        return val;
    }
    T& operator*() { return *operator->(); }

    operator bool() { return has_value; }

private:
    T* val;
    bool has_value;
};

template<typename AssetT> class Handle {
public:
    static constexpr auto noid = static_cast<u64>(-1);

    Handle() noexcept : id(noid) {}
    Handle(u64 id) noexcept : id(id) {}
    Handle(std::nullptr_t) noexcept : id(noid) {}
    Handle& operator=(std::nullptr_t) {
        id = noid;
        return *this;
    }
    Expected<AssetT> get() noexcept {
        if (id == noid)
            return nullptr;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return nullptr;
        else
            return &asset_it->second;
    }
    Expected<const AssetT> get() const noexcept {
        if (id == noid)
            return nullptr;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return nullptr;
        else
            return &asset_it->second;
    }

    void save(fs::path path) const {
        assets::RawSaveData data = assets::raw_get_save_data(*get().operator->());
        std::fstream f(path);
        f << data.bytestream.rdbuf();
    }

    void unload() noexcept {
        if (id == noid)
            return;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return;
        else {
            assets::raw_unload(asset_it->second);
            container.map.erase(asset_it);
            id = noid;
        }
    }

    [[nodiscard]] bool operator==(Handle const& h) const noexcept { return id == h.id; }

    [[nodiscard]] u64 get_id() const noexcept { return id; }

private:
    u64 id;
};

} // namespace arpiyi

namespace arpiyi::asset_manager {

template<typename AssetT> Handle<AssetT> load(assets::LoadParams<AssetT> const& load_params) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    u64 id_to_use = container.next_id_to_use++;
    AssetT& asset = container.map.emplace(id_to_use, AssetT{}).first->second;
    assets::raw_load(asset, load_params);
    return Handle<AssetT>(id_to_use);
}

template<typename AssetT>
Handle<AssetT> load(assets::LoadParams<AssetT> const& load_params, u64 id_to_use) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    AssetT& asset = container.map.emplace(id_to_use, AssetT{}).first->second;
    assets::raw_load(asset, load_params);
    return Handle<AssetT>(id_to_use);
}

template<typename AssetT> Handle<AssetT> put(AssetT const& asset) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    u64 id_to_use = container.next_id_to_use++;
    container.map.emplace(id_to_use, std::move(asset));
    return Handle<AssetT>(id_to_use);
}

template<typename AssetT, bool update_next_id_to_use = true>
Handle<AssetT> put(AssetT const& asset, u64 id_to_use) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    if constexpr (update_next_id_to_use) {
        container.next_id_to_use = std::max(id_to_use + 1, container.next_id_to_use);
    }
    container.map.emplace(id_to_use, std::move(asset));
    return Handle<AssetT>(id_to_use);
}

} // namespace arpiyi::asset_manager

#endif // ARPIYI_ASSET_MANAGER_HPP
