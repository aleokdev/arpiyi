#include "widgets/pickers.hpp"
#include "tileset_manager.hpp"

#include "asset_manager.hpp"
#include "assets/sprite.hpp"

#include <imgui.h>

namespace arpiyi::widgets {

namespace tileset_picker {
void show(Handle<assets::Tileset>& tileset) {
    auto t = tileset.get();
    if (ImGui::BeginCombo("Tileset", t ? t->name.c_str() : "<Not selected>")) {
        for (auto& ts : tileset_manager::get_tilesets()) {
            std::string selectable_strid = std::to_string(ts.get_id()) + " " + ts.get()->name;
            if (ImGui::Selectable(selectable_strid.c_str(), ts == tileset))
                tileset = ts;

            if (ts == tileset)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}
} // namespace tileset_picker

namespace sprite_picker {
bool show(Handle<assets::Sprite>& spr) {
    bool changed = false;
    std::size_t column = 0;
    std::size_t columns_to_render = ImGui::GetWindowWidth() / 60;
    for (const auto& [id, sprite] : detail::AssetContainer<assets::Sprite>::get_instance().map) {
        ImGui::BeginGroup();
        assert(sprite.texture.get());
        // ImageButtons use the texture ID as their own ID for some bizarre reasons, so we need to
        // override the ID used.
        ImGui::PushID(id + 0xFFFF);
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(sprite.texture.get()->handle),
                               {50, 50}, {sprite.uv_min.x, sprite.uv_min.y},
                               {sprite.uv_max.x, sprite.uv_max.y})) {
            spr = Handle<assets::Sprite>(id);
            changed = true;
        }
        ImGui::PopID();
        ImGui::TextDisabled("%zu", id);
        ImGui::SameLine();
        ImGui::TextUnformatted(sprite.name.data());
        ImGui::EndGroup();

        if (column < columns_to_render - 1) {
            ImGui::SameLine();
            ++column;
        } else
            column = 0;
    }
    return changed;
}
} // namespace sprite_picker

} // namespace arpiyi::widgets