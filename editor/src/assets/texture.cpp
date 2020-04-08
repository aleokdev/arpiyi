#include "assets/texture.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace arpiyi_editor::assets {

namespace texture_file_definitions {

constexpr std::string_view id_json_key = "id";
constexpr std::string_view path_json_key = "path";

} // namespace texture_file_definitions

template<> void raw_save(Texture const& texture, SaveParams<Texture> const& params) {
    constexpr u8 channel_width = 4;
    void* data = malloc(texture.w * texture.h * channel_width);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_write_png(params.path.generic_string().c_str(), texture.w, texture.h, STBI_rgb_alpha, data,
                   texture.w * channel_width);
    free(data);
}

} // namespace arpiyi_editor::assets