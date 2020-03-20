#ifndef ARPIYI_ASSET_MANAGER_HPP
#define ARPIYI_ASSET_MANAGER_HPP

#include <filesystem>
#include <unordered_map>

#include <glad/glad.h>
#include <stb_image.h>

namespace fs = std::filesystem;

namespace arpiyi_editor::assets {

struct Texture {
    unsigned int handle;
    unsigned int w;
    unsigned int h;
};

template<typename T> struct LoadParams;

template<> struct LoadParams<Texture> { fs::path path; };

inline void raw_load(Texture& texture, LoadParams<Texture> const& params) {
    int w, h, channels;
    unsigned char* data = stbi_load(params.path.generic_string().c_str(), &w, &h, &channels, 4);

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_POINT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    texture.handle = tex;
    texture.w = w;
    texture.h = h;
}

inline void raw_unload(Texture& texture) {
    glDeleteTextures(1, &texture.handle);
}

} // namespace arpiyi_editor::assets

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
            throw std::runtime_error("Tried to access null Expected value");
        return value.val;
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

    void unload() {
        if (id == -1)
            return;
        auto& container = detail::AssetContainer<AssetT>::get_instance();
        auto asset_it = container.map.find(id);
        if (asset_it == container.map.end())
            return;
        else
        {
            assets::raw_unload(asset_it->second);
            container.map.erase(asset_it);
        }
    }

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

} // namespace arpiyi_editor::asset_manager

#endif // ARPIYI_ASSET_MANAGER_HPP
