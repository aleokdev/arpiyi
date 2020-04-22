#include "window_list_menu.hpp"
#include <algorithm>

namespace arpiyi::window_list_menu {
std::vector<Entry> window_entries;

void add_entry(Entry entry) { window_entries.emplace_back(entry); }
void delete_entry(void (*render_callback)(bool*)) {
    window_entries.erase(std::find_if(
        window_entries.begin(), window_entries.end(),
        [&render_callback](const auto& entry) { return entry.func == render_callback; }));
}

void show_menu_items() {
    for (auto& entry : window_entries) {
        if (entry.show_list_entry) {
            ImGui::MenuItem(entry.name.data(), nullptr, &entry.open);
        }
    }
}

void render_entries() {
    for (auto& entry : window_entries) {
        if (entry.open) {
            entry.func(&entry.open);
        }
    }
}

} // namespace arpiyi::window_list_menu