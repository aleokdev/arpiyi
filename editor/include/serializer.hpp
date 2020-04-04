#ifndef ARPIYI_SERIALIZER_HPP
#define ARPIYI_SERIALIZER_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi_editor::serializer {

void save_project(fs::path dir);
void load_project(fs::path dir);

}

#endif // ARPIYI_SERIALIZER_HPP
