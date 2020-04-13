#include "widgets/tileset_picker.hpp"
#include "tileset_manager.hpp"

#include <imgui.h>

namespace arpiyi_editor::widgets::tileset_picker {

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

} // namespace arpiyi_editor::widgets::tileset_picker