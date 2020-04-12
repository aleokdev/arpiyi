#include "widgets/inspector.hpp"

#include "util/icons_material_design.hpp"
#include "window_list_menu.hpp"

namespace arpiyi_editor::widgets::inspector {

struct SelectedAsset {
    u64 id;
    Handle<assets::Entity> get_entity() { return id; }
    Handle<assets::Map::Comment> get_comment() { return id; }

    enum class Type { entity, comment } type;
} static selection;

void draw_entity_inspector(assets::Entity& entity) {
    char buf[assets::Entity::name_length_limit];
    strcpy(buf, entity.name.c_str());
    if (ImGui::InputText("Name", buf, 32))
        entity.name = buf;
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

} // namespace arpiyi_editor::widgets::inspector