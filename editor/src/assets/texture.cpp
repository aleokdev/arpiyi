#include "assets/texture.hpp"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stb_image_write.h>

namespace arpiyi_editor::assets {

namespace texture_file_definitions {

constexpr std::string_view id_json_key = "id";
constexpr std::string_view path_json_key = "path";

} // namespace texture_file_definitions

template<> RawSaveData raw_get_save_data<Texture>(Texture const& texture) {
    RawSaveData data;
    constexpr u8 channel_width = 4;
    u64 raw_data_size = texture.w * texture.h * channel_width;
    void* raw_tex_data = malloc(raw_data_size);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_tex_data);
    int png_encoded_data_size;
    unsigned const char* png_encoded_data = stbi_write_png_to_mem(reinterpret_cast<unsigned const char*>(raw_tex_data), texture.w * channel_width, texture.w, texture.h, STBI_rgb_alpha, &png_encoded_data_size);
    free(raw_tex_data);

    data.bytestream.write(reinterpret_cast<const char*>(png_encoded_data), png_encoded_data_size);

    return data;
}

} // namespace arpiyi_editor::assets