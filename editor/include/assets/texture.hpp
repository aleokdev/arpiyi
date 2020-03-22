#ifndef ARPIYI_EDITOR_TEXTURE_HPP
#define ARPIYI_EDITOR_TEXTURE_HPP

#include "asset.hpp"

#include <filesystem>

#include <glad/glad.h>
#include <stb_image.h>

namespace fs = std::filesystem;

namespace arpiyi_editor::assets {

struct Texture {
    unsigned int handle;
    unsigned int w;
    unsigned int h;
};

template<> struct LoadParams<Texture> {
    fs::path path;
    bool flip = true;
};

template<> inline void raw_load(Texture& texture, LoadParams<Texture> const& params) {
    stbi_set_flip_vertically_on_load(params.flip);
    int w, h, channels;
    unsigned char* data = stbi_load(params.path.generic_string().c_str(), &w, &h, &channels, 4);

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    // Disable filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    texture.handle = tex;
    texture.w = w;
    texture.h = h;
}

template<> inline void raw_unload(Texture& texture) { glDeleteTextures(1, &texture.handle); }

} // namespace arpiyi_editor::assets

#endif // ARPIYI_EDITOR_TEXTURE_HPP