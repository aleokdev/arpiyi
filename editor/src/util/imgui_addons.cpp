#include "util/imgui_addons.hpp"
#include <imgui_internal.h>

namespace ImGui {

bool Button(const char* label, bool enabled, const ImVec2& size) {
    bool clicked = false;
    if (!enabled) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    if (ImGui::Button("OK", size)) {
        clicked = true;
    }
    if (!enabled) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    return clicked;
}

}