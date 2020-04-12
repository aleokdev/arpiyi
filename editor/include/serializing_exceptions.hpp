#ifndef ARPIYI_SERIALIZING_EXCEPTIONS_HPP
#define ARPIYI_SERIALIZING_EXCEPTIONS_HPP

#include "project_info.hpp"
#include <exception>
#include <filesystem>

namespace fs = std::filesystem;

namespace arpiyi_editor::exceptions {

class EditorVersionDiffers : public std::exception {
public:
    EditorVersionDiffers(fs::path const& project_loading, std::string const& project_version) :
        project_being_loaded(project_loading), project_version(project_version) {
        what_str = "Editor version differs from project version. Project has version ";
        what_str += project_version;
        what_str += ", current editor version is ";
        what_str += ARPIYI_EDITOR_VERSION ".";
    }
    ~EditorVersionDiffers() override = default;

    [[nodiscard]] const char* what() const noexcept override {
        return what_str.c_str();
    }

    fs::path project_being_loaded;
    std::string project_version;

private:
    std::string what_str;
};

} // namespace arpiyi_editor::exceptions

#endif // ARPIYI_SERIALIZING_EXCEPTIONS_HPP
