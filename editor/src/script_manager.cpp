#include "script_manager.hpp"
#include "assets/script.hpp"
#include "util/icons_material_design.hpp"
#include "window_list_menu.hpp"

#include <TextEditor.h>
#include <sol/sol.hpp>

#include <asset_manager.hpp>
#include <charconv>
#include <lua.hpp>

namespace arpiyi::script_editor {

static TextEditor editor;
static ImFont* code_font;
static sol::state lua_linter;
static Handle<assets::Script> selected_script;
static Handle<assets::Script> startup_script;

static void check_for_errors_in_editor_script() {
    const std::string editor_text = editor.GetText();
    sol::load_result result = lua_linter.load(editor_text, "script");
    if (!result.valid()) {
        sol::error err = result;
        constexpr std::string_view ignored_error_start = "[string \"script\"]:";
        std::string_view partial_lineno_view =
            std::string_view(err.what()).substr(ignored_error_start.size());
        std::string_view lineno_view =
            partial_lineno_view.substr(0, partial_lineno_view.rfind(':'));
        int lineno;
        std::from_chars(lineno_view.data(), lineno_view.data() + lineno_view.size(), lineno);
        editor.SetErrorMarkers({{std::min(lineno, editor.GetTotalLines()), err.what()}});
    } else
        editor.SetErrorMarkers({});
}

void init() {
    editor.SetShowWhitespaces(false);
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    ImFontConfig conf;
    conf.RasterizerMultiply = 1.3f;
    code_font = ImGui::GetIO().Fonts->AddFontFromFileTTF("data/monaco.ttf", 21.0f, &conf);
    window_list_menu::add_entry({"Lua Editor", &render});
}

void render(bool* p_show) {
    if (ImGui::Begin(ICON_MD_MEMORY " Lua Editor", p_show)) {
        if (auto script = selected_script.get()) {
            ImGui::PushFont(code_font);
            editor.Render("editor");
            ImGui::PopFont();
            if (editor.IsTextChanged()) {
                check_for_errors_in_editor_script();
                script->source = editor.GetText();
            }
        } else {
            ImGui::TextDisabled("No script selected");
        }
    }
    ImGui::End();

    static bool show_new_script = false;
    if (ImGui::Begin(ICON_MD_MEMORY " Script List", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("New")) {
                asset_manager::put<assets::Script>(
                    {"Script " +
                     std::to_string(
                         detail::AssetContainer<assets::Script>::get_instance().next_id_to_use)});
            }
            ImGui::EndMenuBar();
        }
        if (detail::AssetContainer<assets::Script>::get_instance().map.empty())
            ImGui::TextDisabled("No scripts");
        else {
            u64 id_to_delete = -1;
            for (auto& [_id, _s] : detail::AssetContainer<assets::Script>::get_instance().map) {
                ImGui::TextDisabled("%zu", _id);
                ImGui::SameLine();
                {
                    std::string str_id;
                    if (_id == startup_script.get_id()) {
                        str_id = ICON_MD_PLAY_ARROW + _s.name + "###" + std::to_string(_id);
                    } else {
                        str_id = _s.name + "###" + std::to_string(_id);
                    }
                    if (ImGui::Selectable(str_id.c_str(), _id == selected_script.get_id())) {
                        selected_script = Handle<assets::Script>(_id);
                        editor.SetText(_s.source);
                        check_for_errors_in_editor_script();
                    }
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::Selectable("Set as startup script")) {
                        startup_script = _id;
                    }
                    if (ImGui::Selectable("Delete")) {
                        id_to_delete = _id;
                        selected_script = Handle<assets::Script>(Handle<assets::Script>::noid);
                    }
                    ImGui::EndPopup();
                }
            }

            if (id_to_delete != -1) {
                detail::AssetContainer<assets::Script>::get_instance().map.erase(id_to_delete);
            }
        }
    }
    ImGui::End();

    if (show_new_script) {
        ImGui::SetNextWindowSize({300, 200}, ImGuiCond_Once);
        if (ImGui::Begin(ICON_MD_ADD " New Script", &show_new_script)) {}
        ImGui::End();
    }
}

} // namespace arpiyi::script_editor