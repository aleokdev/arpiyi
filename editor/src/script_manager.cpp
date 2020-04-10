#include "script_manager.hpp"
#include "util/icons_material_design.hpp"

#include <TextEditor.h>
#include <sol/sol.hpp>

#include <charconv>
#include <lua.hpp>

namespace arpiyi_editor::script_editor {

static TextEditor editor;
ImFont* code_font;
static sol::state lua;

void init() {
    editor.SetShowWhitespaces(false);
    editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    ImFontConfig conf;
    conf.RasterizerMultiply = 1.3f;
    code_font = ImGui::GetIO().Fonts->AddFontFromFileTTF("data/monaco.ttf", 21.0f, &conf);
}

void render() {
    if(ImGui::Begin(ICON_MD_MEMORY " Lua Editor")) {
        ImGui::PushFont(code_font);
        editor.Render("editor");
        if(editor.IsTextChanged()) {
            const std::string editor_text = editor.GetText();
            sol::load_result result = lua.load(editor_text, "script");
            if(!result.valid()) {
                sol::error err = result;
                constexpr std::string_view ignored_error_start = "[string \"script\"]:";
                std::string_view partial_lineno_view = std::string_view(err.what()).substr(ignored_error_start.size());
                std::string_view lineno_view = partial_lineno_view.substr(0, partial_lineno_view.rfind(':'));
                int lineno;
                std::from_chars(lineno_view.data(), lineno_view.data() + lineno_view.size(), lineno);
                editor.SetErrorMarkers({{std::min(lineno, editor.GetTotalLines()), err.what()}});
            }
            else
                editor.SetErrorMarkers({});

        }
        ImGui::PopFont();
    }
    ImGui::End();

    if(ImGui::Begin(ICON_MD_MEMORY " Script List", nullptr, ImGuiWindowFlags_MenuBar)) {

    }
    ImGui::End();
}

}