#include "assets/texture.hpp"

#include <fstream>

namespace arpiyi::assets {

template<> void raw_load(TextureAsset& texture, LoadParams<TextureAsset> const& params) {
    texture.handle.unload();
    texture.handle = renderer::TextureHandle::from_file(params.path, params.filter, params.flip);
    texture.load_path = params.path;
}

template<> RawSaveData raw_get_save_data<TextureAsset>(TextureAsset const& texture) {
    RawSaveData save_data;
    std::ifstream f(texture.load_path);
    save_data.bytestream << f.rdbuf();
    f.close();

    return save_data;
}

template<> void raw_unload(TextureAsset& texture) {
    texture.handle.unload();
}

}