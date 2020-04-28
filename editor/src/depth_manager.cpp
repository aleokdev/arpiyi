#include "depth_manager.hpp"
#include "global_tile_size.hpp"
#include "tileset_manager.hpp"
#include "window_list_menu.hpp"

#include "util/icons_material_design.hpp"

namespace arpiyi::depth_manager {

void init() { window_list_menu::add_entry({"Shadow Editor", &render}); }

void render(bool* p_show) {
    if (ImGui::Begin(ICON_MD_FLIP_TO_BACK " Shadow Editor", p_show)) {
        if (auto selected_tileset = tileset_manager::get_selection().tileset.get()) {
            const auto selection_tile = tileset_manager::get_selection().selection_start;

            const auto ts = *selected_tileset;
            const auto uv = ts.get_uv(ts.get_id(selection_tile));
            assert(ts.texture.get());
            ImGui::Image(reinterpret_cast<ImTextureID>(ts.texture.get()->handle),
                         {global_tile_size::get() * 5.f, global_tile_size::get() * 5.f},
                         {uv.start.x, uv.start.y}, {uv.end.x, uv.end.y});
        }
    }

    ImGui::End();
}

} // namespace arpiyi::depth_manager