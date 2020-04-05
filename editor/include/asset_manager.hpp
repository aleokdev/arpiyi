#ifndef ARPIYI_ASSET_MANAGER_HPP
#define ARPIYI_ASSET_MANAGER_HPP

#include "assets/asset.hpp"

#include <cassert>
#include <unordered_map>
#include "util/intdef.hpp"

namespace arpiyi_editor {

namespace detail {

template<typename AssetT> struct AssetContainer {
    std::unordered_map<std::size_t, AssetT> map;
    u64 last_id;

    static AssetContainer& get_instance() {
        static AssetContainer<AssetT> instance;
        return instance;
    }
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
    T& operator*() {
        return *operator->();
    }

    operator bool() { return has_value; }

private:
    T* val;
    bool has_value;
};

template<typename AssetT> class Handle {
public:
    static constexpr auto noid = static_cast<u64>(-1);

    Handle() noexcept : id(noid) {}
    Handle(std::size_t id) noexcept : id(id) {}
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

    void save(assets::SaveParams<AssetT> const& params) const {
        assets::raw_save(*get().operator->(), params);
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
        }
    }

    [[nodiscard]] bool operator==(Handle const& h) const noexcept {
        return id == h.id;
    }

    [[nodiscard]] u64 get_id() const noexcept { return id; }

private:
    u64 id;
};

}

namespace arpiyi_editor::asset_manager {

template<typename AssetT> Handle<AssetT> load(assets::LoadParams<AssetT> const& load_params) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    u64 id_to_use = container.last_id++;
    AssetT& asset = container.map.emplace(id_to_use, AssetT{}).first->second;
    assets::raw_load(asset, load_params);
    return Handle<AssetT>(id_to_use);
}

template<typename AssetT> Handle<AssetT> put(AssetT const& asset) {
    auto& container = detail::AssetContainer<AssetT>::get_instance();
    u64 id_to_use = container.last_id++;
    container.map.emplace(id_to_use, std::move(asset));
    return Handle<AssetT>(id_to_use);
}

} // namespace arpiyi_editor::asset_manager

#endif // ARPIYI_ASSET_MANAGER_HPP
