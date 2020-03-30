#ifndef ARPIYI_ASSET_MANAGER_HPP
#define ARPIYI_ASSET_MANAGER_HPP

#include "assets/asset.hpp"

#include <cassert>
#include <unordered_map>

namespace arpiyi_editor::asset_manager {

namespace detail {

template<typename AssetT> struct AssetContainer {
    std::unordered_map<std::size_t, AssetT> map;
    std::size_t last_id;

    static AssetContainer& get_instance() {
        static AssetContainer<AssetT> instance;
        return instance;
    }
};

} // namespace detail

template<typename T> class Expected {
public:
    Expected(std::nullptr_t) : value{0}, has_value{false} {}
    Expected(T* val) : value{val}, has_value{true} {}

    T* operator->() {
        if (!has_value)
            assert("Tried to access null Expected value");
        return value.val;
    }
    T& operator*() {
        return *operator->();
    }

    operator bool() { return has_value; }

private:
    union {
        T* val;
        char unused;
    } value;
    bool has_value;
};

template<typename AssetT> class Handle {
public:
    Handle() : id(-1) {}
    Handle(std::size_t id) : id(id) {}
    Expected<AssetT> get() {
        if (id == -1)
            return nullptr;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return nullptr;
        else
            return &asset_it->second;
    }
    Expected<const AssetT> const_get() const {
        if (id == -1)
            return nullptr;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return nullptr;
        else
            return &asset_it->second;
    }

    void save(assets::SaveParams<AssetT> const& params) {
        assets::raw_save(*get().operator->(), params);
    }

    void unload() {
        if (id == -1)
            return;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return;
        else {
            assets::raw_unload(asset_it->second);
            container.map.erase(asset_it);
        }
    }

    bool operator==(Handle const& h) const {
        return id == h.id;
    }

    [[nodiscard]] std::size_t get_id() const { return id; }

private:
    std::size_t id;
};

template<typename AssetT> Handle<AssetT> load(assets::LoadParams<AssetT> const& load_params) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    std::size_t id_to_use = container.last_id++;
    AssetT& asset = container.map.emplace(id_to_use, AssetT{}).first->second;
    assets::raw_load(asset, load_params);
    return Handle<AssetT>(id_to_use);
}

template<typename AssetT> Handle<AssetT> put(AssetT const& asset) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    std::size_t id_to_use = container.last_id++;
    container.map.emplace(id_to_use, std::move(asset)).first->second;
    return Handle<AssetT>(id_to_use);
}

} // namespace arpiyi_editor::asset_manager

#endif // ARPIYI_ASSET_MANAGER_HPP
