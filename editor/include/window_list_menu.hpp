#ifndef ARPIYI_WINDOW_LIST_MENU_HPP
#define ARPIYI_WINDOW_LIST_MENU_HPP

#include <imgui.h>
#include <string_view>
#include <vector>

namespace arpiyi_editor::window_list_menu {

struct Entry {
    std::string_view name;
    void(*func)(bool*);
    bool open = true;
};

void add_entry(Entry entry);

/// Generates an ImGui MenuItem for each entry in the entry vector.
void show_menu_items();

void render_entries();

} // namespace arpiyi_editor::window_list_menu

#endif // ARPIYI_WINDOW_LIST_MENU_HPP
