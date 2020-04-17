#include "window_list_menu.hpp"

namespace arpiyi::window_list_menu {
std::vector<Entry> window_entries;

void add_entry(Entry entry) { window_entries.emplace_back(entry); }

void show_menu_items() {
    for (auto& entry : window_entries) { ImGui::MenuItem(entry.name.data(), nullptr, &entry.open); }
}

void render_entries() {
    for (auto& entry : window_entries) {
        if (entry.open) {
            entry.func(&entry.open);
        }
    }
}

} // namespace arpiyi::window_list_menu