#include "widgets/inspector.hpp"

#include "widgets/pickers.hpp"
#include <algorithm>

#include "script_manager.hpp"
#include "util/icons_material_design.hpp"
#include "window_list_menu.hpp"

namespace arpiyi::widgets::inspector {

struct SelectedAsset {
    u64 id;
    Handle<assets::Entity> get_entity() { return id; }
    Handle<assets::Map::Comment> get_comment() { return id; }

    enum class Type { entity, comment } type;
} static selection;

void draw_entity_inspector(assets::Entity& entity) {
    static bool show_sprite_selector = false;

    char buf[assets::Entity::name_length_limit];
    strcpy(buf, entity.name.c_str());

    ImVec2 img_size = {ImGui::GetContentRegionAvailWidth() / 4.f,
                             ImGui::GetContentRegionAvailWidth() / 4.f};
    if (auto s = entity.sprite.get()) {
        auto size_in_pixels = s->get_size_in_pixels();
        float max_axis_size = std::max(size_in_pixels.x, size_in_pixels.y);
        img_size = {img_size.x * size_in_pixels.x / max_axis_size, img_size.y * size_in_pixels.y / max_axis_size};
    }
    if (auto s = entity.sprite.get()) {
        if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(s->texture.get()->handle), img_size,
                               {s->uv_min.x, s->uv_min.y}, {s->uv_max.x, s->uv_max.y}, 1)) {
            show_sprite_selector = true;
        }
    } else {
        if (ImGui::ImageButton(nullptr, img_size, {}, {}, 1)) {
            show_sprite_selector = true;
        }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Sprite");

    if (ImGui::InputText("Name", buf, 32))
        entity.name = buf;

    ImGui::TextUnformatted("Scripts");
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
    ImGui::BeginChild(
        "##_scripts",
        {0, ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(entity.scripts.size() + 2)},
        true);
    Handle<assets::Script> script_to_erase;
    std::size_t i = 0;
    for (const auto& s : entity.scripts) {
        assert(s.get());
        const auto& script = *s.get();

        const auto text_cursor_pos = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton(std::to_string(i).c_str(), {ImGui::GetContentRegionAvailWidth(),
                                                               ImGui::GetTextLineHeight()})) {
            script_to_erase = s;
        }
        ImGui::SetCursorScreenPos(text_cursor_pos);
        if (ImGui::IsItemHovered()) {
            ImGui::Text(ICON_MD_DELETE " %s", script.name.c_str());
        } else {
            ImGui::TextUnformatted(script.name.c_str());
        }
        ++i;
    }

    if (script_to_erase.get()) {
        entity.scripts.erase(
            std::remove(entity.scripts.begin(), entity.scripts.end(), script_to_erase));
    }

    if (ImGui::BeginCombo(ICON_MD_ADD, "")) {
        for (auto& [id, script] : detail::AssetContainer<assets::Script>::get_instance().map) {
            std::string selectable_strid = std::to_string(id) + " " + script.name;
            if (ImGui::Selectable(selectable_strid.c_str())) {
                entity.scripts.emplace_back(Handle<assets::Script>(id));
            }
        }
        ImGui::EndCombo();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();

    if (show_sprite_selector) {
        if (ImGui::Begin(ICON_MD_ADD "Sprite Selector", &show_sprite_selector)) {
            show_sprite_selector = !widgets::sprite_picker::show(entity.sprite);
        }
        ImGui::End();
    }
}

void draw_comment_inspector(assets::Map::Comment& comment) {
    char buf[256];
    strcpy(buf, comment.text.c_str());
    if (ImGui::InputTextMultiline("Text", buf, 256))
        comment.text = buf;
}

void init() { window_list_menu::add_entry({"Inspector", &render}); }

void render(bool* p_open) {
    if (ImGui::Begin(ICON_MD_SETTINGS_APPLICATIONS " Inspector")) {
        switch (selection.type) {
            case SelectedAsset::Type::entity: {
                if (auto e = selection.get_entity().get()) {
                    ImGui::Text("Entity");
                    ImGui::SameLine();
                    ImGui::TextDisabled("ID %zu", selection.id);

                    draw_entity_inspector(*e);
                }
            } break;

            case SelectedAsset::Type::comment: {
                if (auto c = selection.get_comment().get()) {
                    ImGui::Text("Comment");
                    ImGui::SameLine();
                    ImGui::TextDisabled("ID %zu", selection.id);

                    draw_comment_inspector(*c);
                }
            } break;
        }
    }
    ImGui::End();
}

void set_inspected_asset(Handle<assets::Entity> entity) {
    selection.id = entity.get_id();
    selection.type = SelectedAsset::Type::entity;
}
void set_inspected_asset(Handle<assets::Map::Comment> comment) {
    selection.id = comment.get_id();
    selection.type = SelectedAsset::Type::comment;
}

} // namespace arpiyi::widgets::inspector